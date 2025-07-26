#include "audio_output.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "AUDIO_OUTPUT"

// I2S configuration for M5Stack devices
#define I2S_NUM         I2S_NUM_0
#define I2S_BCK_PIN     12  // I2S bit clock pin
#define I2S_WS_PIN      0   // I2S word select pin
#define I2S_DATA_PIN    2   // I2S data pin
#define I2S_BUFFER_SIZE 2048

static SemaphoreHandle_t s_audio_mutex = NULL;
static bool s_is_initialized = false;
static bool s_is_playing = false;

esp_err_t audio_output_init(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channels) {
    if (s_is_initialized) {
        ESP_LOGW(TAG, "Audio output already initialized");
        return ESP_OK;
    }

    // Create mutex for thread safety
    s_audio_mutex = xSemaphoreCreateMutex();
    if (!s_audio_mutex) {
        ESP_LOGE(TAG, "Failed to create audio mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize I2S with the given configuration
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = sample_rate,
        .bits_per_sample = bits_per_sample,
        .channel_format = (channels == 1) ? I2S_CHANNEL_FMT_ONLY_RIGHT : I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_BUFFER_SIZE / 4,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %d", ret);
        vSemaphoreDelete(s_audio_mutex);
        s_audio_mutex = NULL;
        return ret;
    }

    ret = i2s_set_pin(I2S_NUM, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %d", ret);
        i2s_driver_uninstall(I2S_NUM);
        vSemaphoreDelete(s_audio_mutex);
        s_audio_mutex = NULL;
        return ret;
    }

    s_is_initialized = true;
    s_is_playing = false;
    ESP_LOGI(TAG, "Audio output initialized: %lu Hz, %u bits, %u channels", 
             sample_rate, bits_per_sample, channels);
    return ESP_OK;
}

void audio_output_deinit(void) {
    if (!s_is_initialized) {
        return;
    }

    if (xSemaphoreTake(s_audio_mutex, portMAX_DELAY) == pdTRUE) {
        i2s_driver_uninstall(I2S_NUM);
        s_is_initialized = false;
        s_is_playing = false;
        xSemaphoreGive(s_audio_mutex);
    }

    vSemaphoreDelete(s_audio_mutex);
    s_audio_mutex = NULL;

    ESP_LOGI(TAG, "Audio output deinitialized");
}

int audio_output_write(const void* data, size_t size, bool wait_for_completion) {
    if (!s_is_initialized || !data || size == 0) {
        return -1;
    }

    int bytes_written = 0;

    if (xSemaphoreTake(s_audio_mutex, portMAX_DELAY) == pdTRUE) {
        s_is_playing = true;
        
        // Write data to I2S
        size_t bytes_written_temp = 0;
        esp_err_t ret = i2s_write(I2S_NUM, data, size, &bytes_written_temp, wait_for_completion ? portMAX_DELAY : 0);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write audio data: %d", ret);
            bytes_written = -1;
        } else {
            bytes_written = bytes_written_temp;
            
            // If not waiting for completion, we consider it "not playing" after data is queued
            if (!wait_for_completion) {
                s_is_playing = false;
            }
        }
        
        xSemaphoreGive(s_audio_mutex);
    }

    // If we're waiting for completion, wait until all bytes are played
    if (wait_for_completion && bytes_written > 0) {
        size_t bytes_remaining = 0;
        do {
            i2s_get_tx_buffer_state(I2S_NUM, &bytes_remaining);
            if (bytes_remaining > 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        } while (bytes_remaining > 0);
        
        if (xSemaphoreTake(s_audio_mutex, portMAX_DELAY) == pdTRUE) {
            s_is_playing = false;
            xSemaphoreGive(s_audio_mutex);
        }
    }

    return bytes_written;
}

bool audio_output_is_busy(void) {
    bool is_busy = false;
    
    if (s_is_initialized && s_audio_mutex) {
        if (xSemaphoreTake(s_audio_mutex, portMAX_DELAY) == pdTRUE) {
            is_busy = s_is_playing;
            xSemaphoreGive(s_audio_mutex);
        }
    }
    
    return is_busy;
}

bool audio_output_wait_completion(uint32_t timeout_ms) {
    if (!s_is_initialized) {
        return true;  // Not initialized means not playing
    }
    
    TickType_t start_ticks = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    bool timed_out = false;
    
    while (audio_output_is_busy()) {
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Check for timeout if specified
        if (timeout_ms > 0) {
            TickType_t elapsed_ticks = xTaskGetTickCount() - start_ticks;
            if (elapsed_ticks >= timeout_ticks) {
                timed_out = true;
                break;
            }
        }
    }
    
    return !timed_out;
}
