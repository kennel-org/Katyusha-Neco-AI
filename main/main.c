#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_GPIO GPIO_NUM_0  // AtomS3のボタンはGPIO0
#define TAG "MAIN"
#include "openai_rt.h"
#include "sleep_mgr.h"



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
    while (1) {
        int level = gpio_get_level(BUTTON_GPIO);
        if (last_level == 1 && level == 0) {
            // 短押し検出
            openai_rt_start_conversation();
            vTaskDelay(pdMS_TO_TICKS(300)); // チャタリング防止
        }
        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    sleep_mgr_init(60);  // default 60s
    xTaskCreate(&button_task, "button_task", 2048, NULL, 5, NULL);
}
