#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_MODE_OFF,
    LED_MODE_BREATH,
    LED_MODE_RAINBOW,
    LED_MODE_BLINK,
} led_mode_t;

void led_ctrl_init(void);
void led_ctrl_set_mode(led_mode_t mode);

#ifdef __cplusplus
}
#endif
