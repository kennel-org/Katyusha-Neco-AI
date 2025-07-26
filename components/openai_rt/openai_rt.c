#include "openai_rt.h"
#include "esp_log.h"
#include "openai_rt_sdk_stub.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "sleep_mgr.h"
#include "led_ctrl.h"
#include "avatar.h"
#include "esp_timer.h"
#include "audio_output.h"
#include "mic_input.h"

#define TAG "OPENAI_RT"

// Event group bits
#define OPENAI_RT_EVENT_STOP_REQUEST    (1 << 0)
#define OPENAI_RT_EVENT_CONVERSATION_END (1 << 1)

// Maximum conversation time in milliseconds (2 minutes)
#define MAX_CONVERSATION_TIME_MS (2 * 60 * 1000)

typedef struct {
    EventGroupHandle_t event_group;
    esp_timer_handle_t timeout_timer;
    openai_rt_handle_t sdk_handle;
    bool is_active;
    bool mic_initialized;
} openai_rt_context_t;

static TaskHandle_t s_task = NULL;
static openai_rt_context_t s_context = {0};

// Forward declarations
static void conversation_task(void* pvParameters);
static void cleanup_resources(openai_rt_context_t* context);
static void timeout_callback(void* arg);
static void audio_data_callback(const void* audio_data, size_t data_size, void* user_data);
static void conversation_end_callback(void* user_data);
static void mic_data_callback(const void* audio_data, size_t data_size, void* user_data);

static void audio_data_callback(const void* audio_data, size_t data_size, void* user_data) {
    ESP_LOGD(TAG, "Received %d bytes of audio data", data_size);
    
    // Reset sleep timer whenever we receive audio data (user is engaged)
    sleep_mgr_reset_timer();
    
    // Send audio data to the audio output component
    // We don't wait for completion here to avoid blocking the callback
    int bytes_written = audio_output_write(audio_data, data_size, false);
    
    if (bytes_written < 0) {
        ESP_LOGW(TAG, "Failed to write audio data to output");
    } else if (bytes_written != data_size) {
        ESP_LOGW(TAG, "Partial write: %d/%d bytes", bytes_written, data_size);
    }
}

// Conversation end callback from OpenAI SDK
static void conversation_end_callback(void* user_data) {
    openai_rt_context_t* ctx = (openai_rt_context_t*)user_data;
    if (ctx && ctx->event_group) {
        xEventGroupSetBits(ctx->event_group, OPENAI_RT_EVENT_CONVERSATION_END);
    }
    ESP_LOGI(TAG, "Conversation ended by OpenAI service");
}

static void timeout_callback(void* arg) {
    openai_rt_context_t* ctx = (openai_rt_context_t*)arg;
    if (ctx && ctx->event_group) {
        xEventGroupSetBits(ctx->event_group, OPENAI_RT_EVENT_STOP_REQUEST);
        ESP_LOGW(TAG, "Conversation timeout reached");
    }
}

// Microphone data callback - forwards audio data to the OpenAI RT SDK
static void mic_data_callback(const void* data, size_t size, void* user_data) {
    openai_rt_context_t* ctx = (openai_rt_context_t*)user_data;
    
    if (!ctx || !ctx->is_active || !ctx->sdk_handle) {
        ESP_LOGW(TAG, "Cannot send mic data - conversation not active");
        return;
    }
    
    // Reset sleep timer whenever we capture microphone data (user is speaking)
    sleep_mgr_reset_timer();
    
    // Send audio data to the OpenAI RT SDK
    int result = openai_rt_send_audio(ctx->sdk_handle, data, size);
    if (result != 0) {
        ESP_LOGW(TAG, "Failed to send audio data to OpenAI RT SDK: %d", result);
    } else {
        ESP_LOGD(TAG, "Sent %d bytes of audio data to OpenAI RT SDK", size);
    }
}

static void cleanup_resources(openai_rt_context_t* ctx) {
    if (!ctx) return;
    
    // Stop microphone input if initialized
    if (ctx->mic_initialized) {
        mic_input_stop();
        mic_input_deinit();
        ctx->mic_initialized = false;
    }
    
    // Stop and delete timeout timer if active
    if (ctx->timeout_timer) {
        esp_timer_stop(ctx->timeout_timer);
        esp_timer_delete(ctx->timeout_timer);
        ctx->timeout_timer = NULL;
    }
    
    // Delete event group if created
    if (ctx->event_group) {
        vEventGroupDelete(ctx->event_group);
        ctx->event_group = NULL;
    }
    
    // SDK handle is cleaned up by the caller
    ctx->sdk_handle = NULL;
    ctx->is_active = false;
}

static void conversation_task(void* pv) {
    // Get application configuration
    extern const app_config_t* config_mgr_get(void);
    const app_config_t* app_cfg = config_mgr_get();
    
    // Create event group for synchronization
    s_context.event_group = xEventGroupCreate();
    if (!s_context.event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Create timeout timer
    esp_timer_create_args_t timer_args = {
        .callback = timeout_callback,
        .arg = &s_context,
        .name = "openai_timeout"
    };
    
    if (esp_timer_create(&timer_args, &s_context.timeout_timer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timeout timer");
        cleanup_resources(&s_context);
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize OpenAI RT SDK
    openai_rt_callbacks_t callbacks = {
        .audio_data_cb = audio_data_callback,
        .conversation_end_cb = conversation_end_callback,
        .user_data = &s_context
    };
    
    openai_rt_config_t cfg = {
        .api_key = app_cfg->openai.api_key,
        .voice = app_cfg->openai.voice[0] ? app_cfg->openai.voice : "alloy",
    };
    
    s_context.sdk_handle = openai_rt_init(&cfg);
    if (!s_context.sdk_handle) {
        ESP_LOGE(TAG, "SDK init failed");
        cleanup_resources(&s_context);
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Set callbacks for the SDK
    if (openai_rt_set_callbacks(s_context.sdk_handle, &callbacks) != 0) {
        ESP_LOGE(TAG, "Failed to set callbacks");
        openai_rt_deinit(s_context.sdk_handle);
        cleanup_resources(&s_context);
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize audio output component
    esp_err_t audio_err = audio_output_init(16000, 16, 1); // 16kHz, 16-bit, mono
    if (audio_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio output: %d", audio_err);
        openai_rt_deinit(s_context.sdk_handle);
        cleanup_resources(&s_context);
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize microphone input component
    esp_err_t mic_err = mic_input_init(16000, 16); // 16kHz, 16-bit
    if (mic_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize microphone input: %d", mic_err);
        audio_output_deinit();
        openai_rt_deinit(s_context.sdk_handle);
        cleanup_resources(&s_context);
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    s_context.mic_initialized = true;
    
    // Update UI to show we're in conversation mode
    led_ctrl_set_mode(LED_MODE_RAINBOW);
    avatar_set_expression(AVATAR_EXPRESSION_SPEAKING);
    sleep_mgr_reset_timer(); // cancel sleep while talking
    
    // Start conversation and timer
    s_context.is_active = true;
    if (openai_rt_start(s_context.sdk_handle) != 0) {
        ESP_LOGE(TAG, "Failed to start conversation");
        openai_rt_deinit(s_context.sdk_handle);
        cleanup_resources(&s_context);
        led_ctrl_set_mode(LED_MODE_BREATH);
        avatar_set_expression(AVATAR_EXPRESSION_IDLE);
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Start microphone input
    esp_err_t start_err = mic_input_start(mic_data_callback, 1024, &s_context);
    if (start_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start microphone input: %d", start_err);
        openai_rt_stop(s_context.sdk_handle);
        openai_rt_deinit(s_context.sdk_handle);
        cleanup_resources(&s_context);
        led_ctrl_set_mode(LED_MODE_BREATH);
        avatar_set_expression(AVATAR_EXPRESSION_IDLE);
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Conversation and microphone started successfully");
    
    // Start timeout timer
    esp_timer_start_once(s_context.timeout_timer, MAX_CONVERSATION_TIME_MS * 1000);
    
    // Wait for conversation to end or timeout
    EventBits_t bits = xEventGroupWaitBits(
        s_context.event_group,
        OPENAI_RT_EVENT_STOP_REQUEST | OPENAI_RT_EVENT_CONVERSATION_END,
        pdTRUE,  // Clear bits on exit
        pdFALSE, // Wait for any bit
        portMAX_DELAY);
    
    // Stop conversation
    if (s_context.is_active) {
        openai_rt_stop(s_context.sdk_handle);
    }
    
    // Stop microphone input
    if (s_context.mic_initialized) {
        ESP_LOGI(TAG, "Stopping microphone input");
        mic_input_stop();
    }
    
    // Wait for any remaining audio to finish playing (with a timeout)
    if (audio_output_is_busy()) {
        ESP_LOGI(TAG, "Waiting for audio playback to complete...");
        audio_output_wait_completion(2000); // 2 second timeout
    }
    
    // Cleanup audio output
    audio_output_deinit();
    
    // Cleanup SDK resources
    openai_rt_deinit(s_context.sdk_handle);
    
    // Cleanup other resources
    cleanup_resources(&s_context);
    
    // Return to idle state
    led_ctrl_set_mode(LED_MODE_BREATH);
    avatar_set_expression(AVATAR_EXPRESSION_IDLE);
    sleep_mgr_reset_timer();
    
    ESP_LOGI(TAG, "Conversation finished%s", 
             (bits & OPENAI_RT_EVENT_STOP_REQUEST) ? " (timeout)" : "");
    
    s_task = NULL;
    vTaskDelete(NULL);
}

void openai_rt_start_conversation(void) {
    if (s_task) {
        ESP_LOGW(TAG, "Conversation already running");
        return;
    }
    xTaskCreate(conversation_task, "openai_rt_conv", 8192, NULL, 5, &s_task);
}

void openai_rt_stop_conversation(void) {
    if (!s_task || !s_context.event_group) {
        ESP_LOGW(TAG, "No active conversation to stop");
        return;
    }
    
    // Signal the conversation task to stop
    xEventGroupSetBits(s_context.event_group, OPENAI_RT_EVENT_STOP_REQUEST);
    ESP_LOGI(TAG, "Requested conversation stop");
}
