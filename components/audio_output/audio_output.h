#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize the audio output component
 * 
 * @param sample_rate Sample rate in Hz (e.g., 16000, 44100)
 * @param bits_per_sample Bits per sample (8, 16, 24, 32)
 * @param channels Number of audio channels (1 for mono, 2 for stereo)
 * @return ESP_OK on success, or an error code
 */
esp_err_t audio_output_init(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channels);

/**
 * @brief Deinitialize the audio output component and free resources
 */
void audio_output_deinit(void);

/**
 * @brief Write audio data to the output device
 * 
 * @param data Pointer to audio data buffer
 * @param size Size of audio data in bytes
 * @param wait_for_completion If true, wait until all data is played
 * @return Number of bytes written, or negative error code
 */
int audio_output_write(const void* data, size_t size, bool wait_for_completion);

/**
 * @brief Check if audio output is busy playing
 * 
 * @return true if audio is currently playing, false otherwise
 */
bool audio_output_is_busy(void);

/**
 * @brief Wait for audio playback to complete
 * 
 * @param timeout_ms Timeout in milliseconds, or 0 to wait indefinitely
 * @return true if playback completed, false if timed out
 */
bool audio_output_wait_completion(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
