#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "openai_rt.h"
#include "led_ctrl.h"
#include "avatar.h"
#include "config_mgr.h"
#include "sleep_mgr.h"

#define TAG "TEST_OPENAI_RT"
#define TEST_BUTTON_GPIO GPIO_NUM_0  // AtomS3 button on GPIO0

// Test task to handle button presses
void test_button_task(void *pvParameter) {
    // Configure button GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TEST_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    int last_level = 1;
    int press_duration = 0;
    const int LONG_PRESS_THRESHOLD = 150; // 1.5 seconds (150 * 10ms)
    
    ESP_LOGI(TAG, "Test button task started");
    ESP_LOGI(TAG, "Press button briefly to start conversation");
    ESP_LOGI(TAG, "Long press button to stop conversation");
    
    while (1) {
        int level = gpio_get_level(TEST_BUTTON_GPIO);
        
        // Button is pressed
        if (level == 0) {
            press_duration++;
            
            // Long press detection
            if (press_duration == LONG_PRESS_THRESHOLD) {
                ESP_LOGI(TAG, "Long press detected, stopping conversation");
                openai_rt_stop_conversation();
                led_ctrl_set_mode(LED_MODE_BLINK); // Visual feedback
                vTaskDelay(pdMS_TO_TICKS(500));
                led_ctrl_set_mode(LED_MODE_BREATH);
            }
        } 
        // Button is released
        else if (last_level == 0 && level == 1) {
            // Short press detection (not a long press)
            if (press_duration > 0 && press_duration < LONG_PRESS_THRESHOLD) {
                ESP_LOGI(TAG, "Short press detected, starting conversation");
                openai_rt_start_conversation();
            }
            press_duration = 0;
            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce
        }
        
        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void test_openai_rt(void) {
    ESP_LOGI(TAG, "Starting OpenAI RT integration test");
    
    // Initialize components
    avatar_init();
    led_ctrl_init();
    led_ctrl_set_mode(LED_MODE_BREATH);
    avatar_set_expression(AVATAR_EXPRESSION_IDLE);
    
    // Initialize configuration manager
    config_mgr_init();
    const app_config_t* cfg = config_mgr_get();
    
    // Initialize sleep manager
    sleep_mgr_init(cfg->sleep_timeout_sec);
    
    // Create button task for testing
    xTaskCreate(&test_button_task, "test_button_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Test initialized. Press button to start/stop conversation.");
}

// This function can be called from app_main() to run the test
void run_openai_rt_test(void) {
    test_openai_rt();
}
