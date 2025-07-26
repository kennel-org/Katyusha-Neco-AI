#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Callback function for microphone data
 * 
 * @param data Pointer to audio data buffer
 * @param size Size of audio data in bytes
 * @param user_data User data pointer passed to mic_input_start
 */
typedef void (*mic_input_data_cb_t)(const void* data, size_t size, void* user_data);

/**
 * @brief Initialize the microphone input component
 * 
 * @param sample_rate Sample rate in Hz (e.g., 16000)
 * @param bits_per_sample Bits per sample (8, 16, 24, 32)
 * @return ESP_OK on success, or an error code
 */
esp_err_t mic_input_init(uint32_t sample_rate, uint8_t bits_per_sample);

/**
 * @brief Deinitialize the microphone input component and free resources
 */
void mic_input_deinit(void);

/**
 * @brief Start capturing audio from the microphone
 * 
 * @param callback Callback function to receive audio data
 * @param buffer_size Size of each buffer chunk to deliver to the callback
 * @param user_data User data to pass to the callback
 * @return ESP_OK on success, or an error code
 */
esp_err_t mic_input_start(mic_input_data_cb_t callback, size_t buffer_size, void* user_data);

/**
 * @brief Stop capturing audio from the microphone
 */
void mic_input_stop(void);

/**
 * @brief Check if microphone is currently capturing
 * 
 * @return true if capturing, false otherwise
 */
bool mic_input_is_active(void);

#ifdef __cplusplus
}
#endif
