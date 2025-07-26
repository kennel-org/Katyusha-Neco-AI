#include "avatar.h"
#include "esp_log.h"
#include "M5AtomS3.h"
#include "M5GFX.h"
#include "Avatar.hpp"

#define TAG "AVATAR"

// Global objects for M5Stack Avatar
static M5GFX display;
static m5avatar::Avatar avatar;
static bool s_initialized = false;

// Map our expression enum to avatar expressions
static const char* expression_map[] = {
    "neutral",  // AVATAR_EXPRESSION_IDLE
    "doubt",    // AVATAR_EXPRESSION_THINKING
    "happy"     // AVATAR_EXPRESSION_SPEAKING
};

void avatar_init(void) {
    if (s_initialized) {
        return;
    }
    
    ESP_LOGI(TAG, "Initializing avatar");
    
    // Initialize M5AtomS3
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // Initialize display
    display.begin();
    display.setRotation(2);  // Adjust rotation as needed
    display.setBrightness(100);
    display.clear();
    
    // Initialize avatar
    avatar.init(&display, "normal");
    avatar.setPosition(display.width() / 2, display.height() / 2);
    avatar.setScale(0.5f);  // Adjust scale as needed for AtomS3
    
    // Set initial expression
    avatar.setExpression(expression_map[AVATAR_EXPRESSION_IDLE]);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Avatar initialized successfully");
}

void avatar_set_expression(avatar_expression_t exp) {
    if (!s_initialized) {
        ESP_LOGW(TAG, "Avatar not initialized");
        return;
    }
    
    if (exp < AVATAR_EXPRESSION_IDLE || exp > AVATAR_EXPRESSION_SPEAKING) {
        ESP_LOGW(TAG, "Invalid expression: %d", exp);
        return;
    }
    
    ESP_LOGI(TAG, "Setting avatar expression: %s", expression_map[exp]);
    avatar.setExpression(expression_map[exp]);
}

void avatar_set_mouth_ratio(float ratio) {
    if (!s_initialized) {
        return;
    }
    
    // Clamp ratio between 0.0 and 1.0
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    
    avatar.setMouthOpenRatio(ratio);
}
