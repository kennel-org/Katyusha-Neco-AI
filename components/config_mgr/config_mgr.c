#include "config_mgr.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <string.h>
#include <stdio.h>

#define TAG "CONFIG_MGR"

static app_config_t s_cfg = {
    .wifi = {"your-ssid", "your-password"},
    .openai = {"sk-xxxxx", "alloy"},
    .sleep_timeout_sec = 60,
};

static void parse_line(const char* line) {
    if (strncmp(line, "ssid:", 5) == 0) {
        sscanf(line + 5, "%31s", s_cfg.wifi.ssid);
    } else if (strncmp(line, "password:", 9) == 0) {
        sscanf(line + 9, "%63s", s_cfg.wifi.password);
    } else if (strncmp(line, "api_key:", 8) == 0) {
        sscanf(line + 8, "%63s", s_cfg.openai.api_key);
    } else if (strncmp(line, "voice:", 6) == 0) {
        sscanf(line + 6, "%15s", s_cfg.openai.voice);
    } else if (strncmp(line, "timeout_sec:", 12) == 0) {
        sscanf(line + 12, "%u", &s_cfg.sleep_timeout_sec);
    }
}

void config_mgr_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 3,
        .format_if_mount_failed = true
    };
    esp_vfs_spiffs_register(&conf);

    FILE* f = fopen("/spiffs/config.yaml", "r");
    if (!f) {
        ESP_LOGW(TAG, "config.yaml not found, using defaults");
        return;
    }
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        parse_line(line);
    }
    fclose(f);
}

const app_config_t* config_mgr_get(void) {
    return &s_cfg;
}
