#pragma once
// Stub definitions to allow compilation without real SDK
// Replace with actual SDK headers when available

#include <stddef.h>

typedef void* openai_rt_handle_t;

// Callback function types
typedef void (*openai_rt_audio_data_cb_t)(const void* audio_data, size_t data_size, void* user_data);
typedef void (*openai_rt_conversation_end_cb_t)(void* user_data);

// Callback structure
typedef struct {
    openai_rt_audio_data_cb_t audio_data_cb;
    openai_rt_conversation_end_cb_t conversation_end_cb;
    void* user_data;
} openai_rt_callbacks_t;

// Configuration structure
typedef struct {
    const char* api_key;
    const char* voice;
} openai_rt_config_t;

// SDK functions
openai_rt_handle_t openai_rt_init(const openai_rt_config_t* config);
int openai_rt_set_callbacks(openai_rt_handle_t handle, const openai_rt_callbacks_t* callbacks);
int openai_rt_start(openai_rt_handle_t handle);
void openai_rt_stop(openai_rt_handle_t handle);
void openai_rt_deinit(openai_rt_handle_t handle);

/**
 * @brief Send audio data from microphone to the OpenAI RT SDK
 * 
 * @param handle OpenAI RT handle
 * @param audio_data Pointer to audio data buffer
 * @param data_size Size of audio data in bytes
 * @return 0 on success, non-zero on failure
 */
int openai_rt_send_audio(openai_rt_handle_t handle, const void* audio_data, size_t data_size);
