# OpenAI RT Microphone Integration Testing Guide

This guide provides instructions for testing the microphone integration with the OpenAI RT component in the Katyusha-Neco-AI project.

## Overview

The OpenAI RT component has been enhanced with microphone input integration, allowing real-time audio streaming from the microphone to the OpenAI RT SDK. This integration includes:

- Initialization and management of the microphone input component
- Audio data callback forwarding to the OpenAI RT SDK
- Proper resource management and cleanup
- Button control for starting and stopping conversations
- LED visual feedback during conversations

## Testing Options

### Option 1: Test Application

A test application has been included in `main/test_openai_rt.c`. To use this test:

1. Open `main/main.c`
2. Uncomment the line `// run_openai_rt_test(); return;` in the `app_main()` function
3. Build and flash the application
4. Press the button briefly to start a conversation
5. Speak into the microphone
6. Long press the button to stop the conversation

### Option 2: Unit Tests

Unit tests have been created in the `test` directory to verify the integration:

1. Build the project with testing enabled:
   ```
   idf.py -p [PORT] build
   ```

2. Run the tests:
   ```
   idf.py -p [PORT] flash monitor
   ```

3. The tests will verify:
   - Microphone initialization and data capture
   - OpenAI RT SDK integration
   - Audio output playback
   - Full conversation flow

## Expected Behavior

When the microphone integration is working correctly, you should observe:

1. **Starting a conversation**:
   - LED changes to rainbow mode
   - Avatar expression changes to speaking
   - Microphone begins capturing audio
   - Audio data is sent to OpenAI RT SDK

2. **During conversation**:
   - Microphone data is continuously sent to OpenAI RT SDK
   - Audio responses from OpenAI RT are played through the speaker
   - Sleep timer is reset when microphone activity is detected

3. **Stopping a conversation**:
   - LED briefly blinks red
   - Microphone input is stopped
   - OpenAI RT conversation is ended
   - LED returns to breathing mode
   - Avatar returns to idle expression

## Troubleshooting

If you encounter issues with the microphone integration:

1. Check the console logs for error messages
2. Verify that the microphone hardware is properly connected
3. Ensure the microphone pin configuration is correct (currently using GPIO 34)
4. Check that the audio format settings match between components (16kHz, 16-bit)

## Implementation Details

The microphone integration is implemented in the following files:

- `components/openai_rt/openai_rt.c`: Main integration code
- `components/openai_rt/openai_rt_sdk_stub.c`: Stub implementation for testing
- `components/openai_rt/openai_rt_sdk_stub.h`: SDK interface definitions
- `main/test_openai_rt.c`: Test application
- `test/test_mic_openai_rt.c`: Unit tests

The microphone callback function (`mic_data_callback`) captures audio data and forwards it to the OpenAI RT SDK using the `openai_rt_send_audio` function.
