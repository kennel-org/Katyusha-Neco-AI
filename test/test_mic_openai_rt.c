#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "openai_rt.h"
#include "mic_input.h"
#include "audio_output.h"
#include "led_ctrl.h"

#define TAG "TEST_MIC_OPENAI"

// Mock callback for testing
static void test_audio_data_cb(const void* data, size_t size, void* user_data) {
    ESP_LOGI(TAG, "Received %d bytes of audio data in test callback", size);
    // In a real test, we could verify the audio data format here
}

TEST_CASE("Test microphone to OpenAI RT integration", "[openai_rt][mic_input]") {
    // Initialize components
    TEST_ESP_OK(audio_output_init(16000, 16, 1));
    TEST_ESP_OK(mic_input_init(16000, 16));
    
    // Initialize OpenAI RT (we'll use our stub implementation)
    openai_rt_config_t config = {
        .api_key = "test_api_key",
        .voice = "test_voice"
    };
    
    openai_rt_handle_t handle = openai_rt_init(&config);
    TEST_ASSERT_NOT_NULL(handle);
    
    // Set up callbacks
    openai_rt_callbacks_t callbacks = {
        .audio_data_cb = test_audio_data_cb,
        .conversation_end_cb = NULL,
        .user_data = NULL
    };
    
    TEST_ASSERT_EQUAL(0, openai_rt_set_callbacks(handle, &callbacks));
    
    // Start conversation
    TEST_ASSERT_EQUAL(0, openai_rt_start(handle));
    
    // Start microphone input with our own test callback
    TEST_ESP_OK(mic_input_start(test_audio_data_cb, 1024, NULL));
    
    // Let it run for a short time
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Stop everything
    mic_input_stop();
    openai_rt_stop(handle);
    
    // Clean up
    openai_rt_deinit(handle);
    mic_input_deinit();
    audio_output_deinit();
    
    ESP_LOGI(TAG, "Microphone to OpenAI RT integration test completed");
}

// Test the full conversation flow
TEST_CASE("Test full conversation flow", "[openai_rt][integration]") {
    // Initialize LED control for visual feedback
    led_ctrl_init();
    
    // Start with breathing mode
    led_ctrl_set_mode(LED_MODE_BREATH);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Start conversation
    openai_rt_start_conversation();
    
    // Verify LED mode changed to rainbow
    vTaskDelay(pdMS_TO_TICKS(100));
    // In a real test, we would verify the LED mode here
    
    // Let the conversation run for a few seconds
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Stop conversation
    openai_rt_stop_conversation();
    
    // Verify LED mode changed to blink and then back to breath
    vTaskDelay(pdMS_TO_TICKS(1000));
    // In a real test, we would verify the LED mode here
    
    ESP_LOGI(TAG, "Full conversation flow test completed");
}
