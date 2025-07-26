#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_GPIO GPIO_NUM_0  // AtomS3のボタンはGPIO0
#define TAG "MAIN"
#include "openai_rt.h"
#include "sleep_mgr.h"
#include "avatar.h"
#include "config_mgr.h"
#include "led_ctrl.h"

// Forward declaration of test function
extern void run_openai_rt_test(void);



void button_task(void *pvParameter) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    int last_level = 1;
    int press_duration = 0;
    const int LONG_PRESS_THRESHOLD = 150; // 1.5秒 (150 * 10ms)
    
    while (1) {
        int level = gpio_get_level(BUTTON_GPIO);
        
        // ボタンが押されている
        if (level == 0) {
            press_duration++;
            
            // 長押し検出
            if (press_duration == LONG_PRESS_THRESHOLD) {
                ESP_LOGI(TAG, "Long press detected, stopping conversation");
                openai_rt_stop_conversation();
                led_ctrl_set_mode(LED_MODE_BLINK); // 視覚的フィードバック
                vTaskDelay(pdMS_TO_TICKS(500));
                led_ctrl_set_mode(LED_MODE_BREATH);
            }
        } 
        // ボタンが離された
        else if (last_level == 0 && level == 1) {
            // 短押し検出 (長押しではない場合)
            if (press_duration > 0 && press_duration < LONG_PRESS_THRESHOLD) {
                ESP_LOGI(TAG, "Short press detected, starting conversation");
                openai_rt_start_conversation();
            }
            press_duration = 0;
            vTaskDelay(pdMS_TO_TICKS(300)); // チャタリング防止
        }
        
        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    // Uncomment the next line to run the OpenAI RT integration test instead of normal operation
    run_openai_rt_test(); return;
    
    // Normal application initialization
    avatar_init();
    led_ctrl_init();
    led_ctrl_set_mode(LED_MODE_BREATH);
    avatar_set_expression(AVATAR_EXPRESSION_IDLE);
    config_mgr_init();
    const app_config_t* cfg = config_mgr_get();
    sleep_mgr_init(cfg->sleep_timeout_sec);
    xTaskCreate(&button_task, "button_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Katyusha-Neco-AI started");
    ESP_LOGI(TAG, "Press button to start conversation, long press to stop");
}
