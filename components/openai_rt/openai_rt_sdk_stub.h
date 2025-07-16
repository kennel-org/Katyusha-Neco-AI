#pragma once
// Stub definitions to allow compilation without real SDK
// Replace with actual SDK headers when available

typedef void* openai_rt_handle_t;

typedef struct {
    const char* api_key;
    const char* voice;
} openai_rt_config_t;

openai_rt_handle_t openai_rt_init(const openai_rt_config_t* config);
void openai_rt_start(openai_rt_handle_t handle);
void openai_rt_stop(openai_rt_handle_t handle);

