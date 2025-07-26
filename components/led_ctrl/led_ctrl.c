#include "led_ctrl.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "driver/gpio.h"
#include <math.h>

#define TAG "LED_CTRL"

// Neco LED configuration
#define LED_GPIO_PIN        GPIO_NUM_38    // GPIO pin connected to Neco LED data line
#define LED_COUNT           70             // Number of LEDs in Neco
#define LED_RMT_CHANNEL     RMT_CHANNEL_0  // RMT channel for LED control

// WS2812B timing (in nanoseconds)
#define LED_T0H             350   // 0 bit high time
#define LED_T0L             900   // 0 bit low time
#define LED_T1H             900   // 1 bit high time
#define LED_T1L             350   // 1 bit low time
#define LED_RESET_TIME      50000 // Reset time

// Color structure for RGB LEDs
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

static led_mode_t s_mode = LED_MODE_OFF;
static TaskHandle_t s_task = NULL;
static rgb_color_t s_led_buffer[LED_COUNT];
static rmt_item32_t s_rmt_items[LED_COUNT * 24 + 1]; // 24 bits per LED + reset
static float s_breath_level = 0.0f;
static float s_breath_direction = 1.0f;
static uint32_t s_rainbow_offset = 0;
static bool s_blink_state = false;
static uint8_t s_blink_count = 0;
static const uint8_t MAX_BLINK_COUNT = 6; // 3 on-off cycles

// Convert RGB values to 24-bit color
static inline uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b) {
    return (g << 16) | (r << 8) | b; // GRB format for WS2812B
}

// Initialize the RMT peripheral for LED control
static void rmt_init(void) {
    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = LED_RMT_CHANNEL,
        .gpio_num = LED_GPIO_PIN,
        .clk_div = 2, // 40MHz / 2 = 20MHz (50ns resolution)
        .mem_block_num = 1,
        .tx_config = {
            .loop_en = false,
            .carrier_en = false,
            .idle_output_en = true,
            .idle_level = RMT_IDLE_LEVEL_LOW,
        }
    };
    
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

// Convert 24-bit color to RMT pulses
static void prepare_rmt_data(void) {
    size_t rmt_item_index = 0;
    
    for (size_t led_index = 0; led_index < LED_COUNT; led_index++) {
        uint32_t color = rgb_to_uint32(
            s_led_buffer[led_index].r,
            s_led_buffer[led_index].g,
            s_led_buffer[led_index].b
        );
        
        // Convert 24 bits to RMT pulses (MSB first)
        for (int bit = 23; bit >= 0; bit--) {
            uint32_t bit_set = (color >> bit) & 1;
            if (bit_set) { // 1 bit
                s_rmt_items[rmt_item_index].level0 = 1;
                s_rmt_items[rmt_item_index].duration0 = LED_T1H / 50; // Convert ns to RMT ticks
                s_rmt_items[rmt_item_index].level1 = 0;
                s_rmt_items[rmt_item_index].duration1 = LED_T1L / 50;
            } else { // 0 bit
                s_rmt_items[rmt_item_index].level0 = 1;
                s_rmt_items[rmt_item_index].duration0 = LED_T0H / 50;
                s_rmt_items[rmt_item_index].level1 = 0;
                s_rmt_items[rmt_item_index].duration1 = LED_T0L / 50;
            }
            rmt_item_index++;
        }
    }
    
    // Add reset code at the end
    s_rmt_items[rmt_item_index].level0 = 0;
    s_rmt_items[rmt_item_index].duration0 = LED_RESET_TIME / 50;
    s_rmt_items[rmt_item_index].level1 = 0;
    s_rmt_items[rmt_item_index].duration1 = 0;
}

// Send LED data to the strip
static void update_leds(void) {
    prepare_rmt_data();
    ESP_ERROR_CHECK(rmt_write_items(LED_RMT_CHANNEL, s_rmt_items, LED_COUNT * 24 + 1, true));
}

// Clear all LEDs (set to black/off)
static void clear_leds(void) {
    for (size_t i = 0; i < LED_COUNT; i++) {
        s_led_buffer[i].r = 0;
        s_led_buffer[i].g = 0;
        s_led_buffer[i].b = 0;
    }
    update_leds();
}

// Set all LEDs to a specific color
static void set_all_leds(uint8_t r, uint8_t g, uint8_t b) {
    for (size_t i = 0; i < LED_COUNT; i++) {
        s_led_buffer[i].r = r;
        s_led_buffer[i].g = g;
        s_led_buffer[i].b = b;
    }
    update_leds();
}

// Generate a color from HSV values (hue: 0-359, sat: 0-255, val: 0-255)
static rgb_color_t hsv_to_rgb(uint16_t hue, uint8_t sat, uint8_t val) {
    rgb_color_t rgb;
    
    // Convert hue to 0-5
    uint8_t i = hue / 60;
    uint8_t f = (hue % 60) * 255 / 60;
    uint8_t p = (val * (255 - sat)) / 255;
    uint8_t q = (val * (255 - (sat * f) / 255)) / 255;
    uint8_t t = (val * (255 - (sat * (255 - f)) / 255)) / 255;
    
    switch (i) {
        case 0: rgb.r = val; rgb.g = t; rgb.b = p; break;
        case 1: rgb.r = q; rgb.g = val; rgb.b = p; break;
        case 2: rgb.r = p; rgb.g = val; rgb.b = t; break;
        case 3: rgb.r = p; rgb.g = q; rgb.b = val; break;
        case 4: rgb.r = t; rgb.g = p; rgb.b = val; break;
        default: rgb.r = val; rgb.g = p; rgb.b = q; break;
    }
    
    return rgb;
}

// Update breathing effect
static void update_breathing_effect(void) {
    // Update breathing level (0.0 to 1.0)
    s_breath_level += 0.01f * s_breath_direction;
    
    if (s_breath_level >= 1.0f) {
        s_breath_level = 1.0f;
        s_breath_direction = -1.0f;
    } else if (s_breath_level <= 0.0f) {
        s_breath_level = 0.0f;
        s_breath_direction = 1.0f;
    }
    
    // Apply sine wave for smoother breathing
    float brightness = (sin(s_breath_level * M_PI) + 1.0f) / 2.0f;
    uint8_t val = (uint8_t)(brightness * 100); // Max brightness 100 to avoid too bright
    
    // Soft blue color for breathing
    set_all_leds(0, val, val);
}

// Update rainbow effect
static void update_rainbow_effect(void) {
    for (size_t i = 0; i < LED_COUNT; i++) {
        // Calculate hue based on position and offset
        uint16_t hue = (i * 360 / LED_COUNT + s_rainbow_offset) % 360;
        rgb_color_t color = hsv_to_rgb(hue, 255, 100); // Saturation 255, Value 100
        
        s_led_buffer[i].r = color.r;
        s_led_buffer[i].g = color.g;
        s_led_buffer[i].b = color.b;
    }
    
    // Update rainbow offset for animation
    s_rainbow_offset = (s_rainbow_offset + 5) % 360;
    
    update_leds();
}

// Update blinking effect (red blink for alerts/notifications)
static void update_blinking_effect(void) {
    if (s_blink_state) {
        // Blink on - bright red
        set_all_leds(150, 0, 0);
    } else {
        // Blink off
        clear_leds();
    }
    
    // Toggle blink state
    s_blink_state = !s_blink_state;
    
    // Increment blink count on state change
    s_blink_count++;
    
    // If we've completed the specified number of blinks, reset to breathing mode
    if (s_blink_count >= MAX_BLINK_COUNT) {
        s_blink_count = 0;
        s_blink_state = false;
        s_mode = LED_MODE_BREATH;
    }
}

static void led_task(void* pv) {
    while (1) {
        switch (s_mode) {
            case LED_MODE_OFF:
                clear_leds();
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            case LED_MODE_BREATH:
                update_breathing_effect();
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
            case LED_MODE_RAINBOW:
                update_rainbow_effect();
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
            case LED_MODE_BLINK:
                update_blinking_effect();
                vTaskDelay(pdMS_TO_TICKS(200)); // Faster blink rate
                break;
        }
    }
}

void led_ctrl_init(void) {
    if (s_task) return;
    
    // Initialize RMT for LED control
    rmt_init();
    
    // Clear LEDs initially
    clear_leds();
    
    ESP_LOGI(TAG, "LED controller initialized with %d LEDs on GPIO %d", LED_COUNT, LED_GPIO_PIN);
    
    // Create LED control task
    xTaskCreate(led_task, "led_task", 4096, NULL, 5, &s_task);
}

void led_ctrl_set_mode(led_mode_t mode) {
    ESP_LOGI(TAG, "Setting LED mode to %d", mode);
    s_mode = mode;
}
