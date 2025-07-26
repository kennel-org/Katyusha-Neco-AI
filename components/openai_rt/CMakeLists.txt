idf_component_register(SRCS "openai_rt.c" "openai_rt_sdk_stub.c"
                       INCLUDE_DIRS "."
                       REQUIRES audio_output mic_input)
