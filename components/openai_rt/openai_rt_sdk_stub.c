#include "openai_rt_sdk_stub.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define TAG "OPENAI_RT_SDK"

// Stub implementation of the OpenAI RT SDK
typedef struct {
    openai_rt_callbacks_t callbacks;
    bool is_active;
    TaskHandle_t response_task;
    esp_timer_handle_t response_timer;
} openai_rt_sdk_context_t;

// Test audio data for simulating responses
static const uint8_t test_audio_data[] = {
    // Simple sine wave pattern (just for testing)
    0x00, 0x00, 0x5A, 0x82, 0x00, 0x00, 0xA6, 0x7D,
    0x00, 0x00, 0x5A, 0x82, 0x00, 0x00, 0xA6, 0x7D,
    0x00, 0x00, 0x5A, 0x82, 0x00, 0x00, 0xA6, 0x7D,
    0x00, 0x00, 0x5A, 0x82, 0x00, 0x00, 0xA6, 0x7D,
};

// Forward declarations
static void send_test_audio_response(void* arg);
static void response_task_func(void* arg);

// Initialize the OpenAI RT SDK
openai_rt_handle_t openai_rt_init(const openai_rt_config_t* config) {
    ESP_LOGI(TAG, "Initializing OpenAI RT SDK (stub)");
    ESP_LOGI(TAG, "API Key: %s...", config->api_key[0] ? "set" : "not set");
    ESP_LOGI(TAG, "Voice: %s", config->voice);
    
    openai_rt_sdk_context_t* ctx = calloc(1, sizeof(openai_rt_sdk_context_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to allocate SDK context");
        return NULL;
    }
    
    return (openai_rt_handle_t)ctx;
}

// Set callbacks for the OpenAI RT SDK
int openai_rt_set_callbacks(openai_rt_handle_t handle, const openai_rt_callbacks_t* callbacks) {
    openai_rt_sdk_context_t* ctx = (openai_rt_sdk_context_t*)handle;
    if (!ctx || !callbacks) return -1;
    
    ctx->callbacks = *callbacks;
    ESP_LOGI(TAG, "Callbacks set");
    return 0;
}

// Start a conversation
int openai_rt_start(openai_rt_handle_t handle) {
    openai_rt_sdk_context_t* ctx = (openai_rt_sdk_context_t*)handle;
    if (!ctx) return -1;
    
    ctx->is_active = true;
    ESP_LOGI(TAG, "Conversation started");
    
    // Create a task to simulate responses
    xTaskCreate(response_task_func, "openai_resp", 4096, ctx, 5, &ctx->response_task);
    
    return 0;
}

// Stop a conversation
void openai_rt_stop(openai_rt_handle_t handle) {
    openai_rt_sdk_context_t* ctx = (openai_rt_sdk_context_t*)handle;
    if (!ctx) return;
    
    ctx->is_active = false;
    ESP_LOGI(TAG, "Conversation stopped");
    
    // Wait for response task to finish
    if (ctx->response_task) {
        vTaskDelay(pdMS_TO_TICKS(100));
        ctx->response_task = NULL;
    }
    
    // Notify that conversation has ended
    if (ctx->callbacks.conversation_end_cb && ctx->callbacks.user_data) {
        ctx->callbacks.conversation_end_cb(ctx->callbacks.user_data);
    }
}

// Send audio data to the OpenAI RT SDK
int openai_rt_send_audio(openai_rt_handle_t handle, const void* audio_data, size_t data_size) {
    openai_rt_sdk_context_t* ctx = (openai_rt_sdk_context_t*)handle;
    if (!ctx || !ctx->is_active) return -1;
    
    ESP_LOGD(TAG, "Received %d bytes of audio data", data_size);
    
    // In a real implementation, this would send the audio data to the OpenAI service
    // For the stub, we'll just log it and trigger a response
    
    return 0;
}

// Deinitialize the OpenAI RT SDK
void openai_rt_deinit(openai_rt_handle_t handle) {
    openai_rt_sdk_context_t* ctx = (openai_rt_sdk_context_t*)handle;
    if (!ctx) return;
    
    // Make sure conversation is stopped
    if (ctx->is_active) {
        openai_rt_stop(handle);
    }
    
    // Free resources
    free(ctx);
    ESP_LOGI(TAG, "SDK deinitialized");
}

// Task to simulate responses from OpenAI
static void response_task_func(void* arg) {
    openai_rt_sdk_context_t* ctx = (openai_rt_sdk_context_t*)arg;
    if (!ctx) {
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Response task started");
    
    // Create a timer to send periodic audio responses
    esp_timer_create_args_t timer_args = {
        .callback = send_test_audio_response,
        .arg = ctx,
        .name = "audio_response"
    };
    
    if (esp_timer_create(&timer_args, &ctx->response_timer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create response timer");
        vTaskDelete(NULL);
        return;
    }
    
    // Start sending responses after a short delay
    esp_timer_start_once(ctx->response_timer, 1000 * 1000); // 1 second
    
    // Wait until conversation is stopped
    while (ctx->is_active) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Clean up timer
    if (ctx->response_timer) {
        esp_timer_stop(ctx->response_timer);
        esp_timer_delete(ctx->response_timer);
        ctx->response_timer = NULL;
    }
    
    ESP_LOGI(TAG, "Response task ended");
    vTaskDelete(NULL);
}

// Timer callback to send test audio responses
static void send_test_audio_response(void* arg) {
    openai_rt_sdk_context_t* ctx = (openai_rt_sdk_context_t*)arg;
    if (!ctx || !ctx->is_active) return;
    
    // Send test audio data
    if (ctx->callbacks.audio_data_cb && ctx->callbacks.user_data) {
        ctx->callbacks.audio_data_cb(test_audio_data, sizeof(test_audio_data), ctx->callbacks.user_data);
    }
    
    // Schedule next response if still active
    if (ctx->is_active) {
        esp_timer_start_once(ctx->response_timer, 500 * 1000); // 500ms
    }
}
