#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    char ssid[32];
    char password[64];
} wifi_config_t;

typedef struct {
    char api_key[64];
    char voice[16];
} openai_config_t;

typedef struct {
    wifi_config_t wifi;
    openai_config_t openai;
    uint32_t sleep_timeout_sec;
} app_config_t;

const app_config_t* config_mgr_get(void);
void config_mgr_init(void);

#ifdef __cplusplus
}
#endif
