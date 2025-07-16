#include "openai_rt.h"
#include "esp_log.h"
#include "openai_rt_sdk_stub.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sleep_mgr.h"

#define TAG "OPENAI_RT"

static TaskHandle_t s_task = NULL;

static void conversation_task(void* pv) {
    openai_rt_config_t cfg = {
        .api_key = "YOUR_API_KEY",
        .voice = "alloy",
    };
    openai_rt_handle_t handle = openai_rt_init(&cfg);
    if (!handle) {
        ESP_LOGE(TAG, "SDK init failed");
        vTaskDelete(NULL);
        return;
    }
    sleep_mgr_reset_timer(); // cancel sleep while talking
    openai_rt_start(handle);
    // ここでは単に10秒待って停止するデモ
    vTaskDelay(pdMS_TO_TICKS(10000));
    openai_rt_stop(handle);
    sleep_mgr_reset_timer();
    ESP_LOGI(TAG, "Conversation finished");
    s_task = NULL;
    vTaskDelete(NULL);
}

void openai_rt_start_conversation(void) {
    if (s_task) {
        ESP_LOGW(TAG, "Conversation already running");
        return;
    }
    xTaskCreate(conversation_task, "openai_rt_conv", 4096, NULL, 5, &s_task);
}
