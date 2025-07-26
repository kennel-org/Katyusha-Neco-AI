#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start a new OpenAI real-time conversation
 * 
 * This function starts a new conversation task that will handle the OpenAI
 * real-time conversation. If a conversation is already running, this function
 * will do nothing.
 */
void openai_rt_start_conversation(void);

/**
 * @brief Stop an ongoing OpenAI real-time conversation
 * 
 * This function signals the conversation task to stop. The task will clean up
 * resources and terminate gracefully.
 */
void openai_rt_stop_conversation(void);

#ifdef __cplusplus
}
#endif
