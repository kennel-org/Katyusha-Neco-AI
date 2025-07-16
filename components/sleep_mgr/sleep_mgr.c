#include "sleep_mgr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#define TAG "SLEEP_MGR"
#define BUTTON_GPIO GPIO_NUM_0

static TimerHandle_t s_timer = NULL;

static void timer_cb(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Timeout reached. Entering deep sleep...");
    // Configure wakeup by button (active LOW)
    esp_sleep_enable_ext0_wakeup(BUTTON_GPIO, 0);
    esp_deep_sleep_start();
}

void sleep_mgr_init(uint32_t timeout_sec) {
    if (s_timer) return;
    s_timer = xTimerCreate("sleep_timer", pdMS_TO_TICKS(timeout_sec * 1000), pdFALSE, NULL, timer_cb);
    if (!s_timer) {
        ESP_LOGE(TAG, "Failed to create timer");
        return;
    }
    xTimerStart(s_timer, 0);
}

void sleep_mgr_reset_timer(void) {
    if (!s_timer) return;
    xTimerStop(s_timer, 0);
    xTimerChangePeriod(s_timer, xTimerGetPeriod(s_timer), 0);
    xTimerStart(s_timer, 0);
}

void sleep_mgr_force_sleep(void) {
    timer_cb(NULL);
}
