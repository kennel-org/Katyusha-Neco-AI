#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void sleep_mgr_init(uint32_t timeout_sec);
void sleep_mgr_reset_timer(void);
void sleep_mgr_force_sleep(void);

#ifdef __cplusplus
}
#endif
