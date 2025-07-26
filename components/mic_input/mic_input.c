#include "mic_input.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "MIC_INPUT"

// I2S configuration for M5Stack microphone
#define I2S_NUM         I2S_NUM_1
#define I2S_MIC_PIN     34  // Default microphone pin for M5Stack
#define I2S_BUFFER_SIZE 2048

typedef struct {
    TaskHandle_t task_handle;
    mic_input_data_cb_t data_callback;
    void* user_data;
    size_t buffer_size;
    bool is_running;
    SemaphoreHandle_t mutex;
} mic_input_context_t;

static mic_input_context_t s_context = {0};

// Forward declaration
static void mic_input_task(void* arg);

esp_err_t mic_input_init(uint32_t sample_rate, uint8_t bits_per_sample) {
    if (s_context.mutex != NULL) {
        ESP_LOGW(TAG, "Microphone input already initialized");
        return ESP_OK;
    }

    // Create mutex for thread safety
    s_context.mutex = xSemaphoreCreateMutex();
    if (!s_context.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize I2S for microphone input
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = sample_rate,
        .bits_per_sample = bits_per_sample,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_BUFFER_SIZE / 4,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_PIN_NO_CHANGE,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_PIN
    };

    esp_err_t ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %d", ret);
        vSemaphoreDelete(s_context.mutex);
        s_context.mutex = NULL;
        return ret;
    }

    ret = i2s_set_pin(I2S_NUM, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %d", ret);
        i2s_driver_uninstall(I2S_NUM);
        vSemaphoreDelete(s_context.mutex);
        s_context.mutex = NULL;
        return ret;
    }

    s_context.is_running = false;
    s_context.task_handle = NULL;
    s_context.data_callback = NULL;
    s_context.user_data = NULL;

    ESP_LOGI(TAG, "Microphone input initialized: %lu Hz, %u bits", 
             sample_rate, bits_per_sample);
    return ESP_OK;
}

void mic_input_deinit(void) {
    if (s_context.mutex == NULL) {
        return;
    }

    // Stop microphone if running
    mic_input_stop();

    // Take mutex to ensure no one is using the microphone
    if (xSemaphoreTake(s_context.mutex, portMAX_DELAY) == pdTRUE) {
        i2s_driver_uninstall(I2S_NUM);
        xSemaphoreGive(s_context.mutex);
    }

    vSemaphoreDelete(s_context.mutex);
    s_context.mutex = NULL;

    ESP_LOGI(TAG, "Microphone input deinitialized");
}

esp_err_t mic_input_start(mic_input_data_cb_t callback, size_t buffer_size, void* user_data) {
    if (s_context.mutex == NULL) {
        ESP_LOGE(TAG, "Microphone input not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (callback == NULL || buffer_size == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    if (xSemaphoreTake(s_context.mutex, portMAX_DELAY) == pdTRUE) {
        if (s_context.is_running) {
            ESP_LOGW(TAG, "Microphone input already running");
            ret = ESP_ERR_INVALID_STATE;
        } else {
            s_context.data_callback = callback;
            s_context.user_data = user_data;
            s_context.buffer_size = buffer_size;
            s_context.is_running = true;

            // Create task to read microphone data
            BaseType_t task_created = xTaskCreate(
                mic_input_task,
                "mic_input_task",
                4096,
                NULL,
                5,
                &s_context.task_handle
            );

            if (task_created != pdPASS) {
                ESP_LOGE(TAG, "Failed to create microphone task");
                s_context.is_running = false;
                ret = ESP_ERR_NO_MEM;
            } else {
                ESP_LOGI(TAG, "Microphone input started");
            }
        }
        xSemaphoreGive(s_context.mutex);
    } else {
        ret = ESP_ERR_TIMEOUT;
    }

    return ret;
}

void mic_input_stop(void) {
    if (s_context.mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(s_context.mutex, portMAX_DELAY) == pdTRUE) {
        if (s_context.is_running && s_context.task_handle != NULL) {
            s_context.is_running = false;
            // Give task time to exit gracefully
            xSemaphoreGive(s_context.mutex);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Delete task if it's still running
            if (s_context.task_handle != NULL) {
                vTaskDelete(s_context.task_handle);
                s_context.task_handle = NULL;
            }
            
            ESP_LOGI(TAG, "Microphone input stopped");
        } else {
            xSemaphoreGive(s_context.mutex);
        }
    }
}

bool mic_input_is_active(void) {
    bool is_active = false;
    
    if (s_context.mutex != NULL) {
        if (xSemaphoreTake(s_context.mutex, portMAX_DELAY) == pdTRUE) {
            is_active = s_context.is_running;
            xSemaphoreGive(s_context.mutex);
        }
    }
    
    return is_active;
}

static void mic_input_task(void* arg) {
    size_t buffer_size = s_context.buffer_size;
    uint8_t* buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_8BIT);
    
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        if (xSemaphoreTake(s_context.mutex, portMAX_DELAY) == pdTRUE) {
            s_context.is_running = false;
            s_context.task_handle = NULL;
            xSemaphoreGive(s_context.mutex);
        }
        vTaskDelete(NULL);
        return;
    }
    
    size_t bytes_read = 0;
    
    while (1) {
        // Check if we should exit
        if (xSemaphoreTake(s_context.mutex, 0) == pdTRUE) {
            if (!s_context.is_running) {
                xSemaphoreGive(s_context.mutex);
                break;
            }
            xSemaphoreGive(s_context.mutex);
        }
        
        // Read data from I2S
        esp_err_t ret = i2s_read(I2S_NUM, buffer, buffer_size, &bytes_read, portMAX_DELAY);
        
        if (ret == ESP_OK && bytes_read > 0) {
            // Call the callback with the data
            if (xSemaphoreTake(s_context.mutex, 0) == pdTRUE) {
                if (s_context.is_running && s_context.data_callback) {
                    s_context.data_callback(buffer, bytes_read, s_context.user_data);
                }
                xSemaphoreGive(s_context.mutex);
            }
        } else if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Error reading from I2S: %d", ret);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    // Cleanup
    heap_caps_free(buffer);
    
    if (xSemaphoreTake(s_context.mutex, portMAX_DELAY) == pdTRUE) {
        s_context.task_handle = NULL;
        xSemaphoreGive(s_context.mutex);
    }
    
    ESP_LOGI(TAG, "Microphone task exiting");
    vTaskDelete(NULL);
}
