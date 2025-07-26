#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AVATAR_EXPRESSION_IDLE,
    AVATAR_EXPRESSION_THINKING,
    AVATAR_EXPRESSION_SPEAKING,
} avatar_expression_t;

void avatar_init(void);
void avatar_set_expression(avatar_expression_t exp);
void avatar_set_mouth_ratio(float ratio);

#ifdef __cplusplus
}
#endif
