#pragma once
#include <stdbool.h>
#include <stdint.h>

#define BSP_USER_BUTTON_DEBOUNCE_MS 30U
typedef struct {
    bool candidate, stable, armed;
    uint32_t since;
} bsp_button_debounce_t;

static inline void bsp_button_debounce_init(bsp_button_debounce_t *state, bool pressed, uint32_t now)
{
    *state = (bsp_button_debounce_t){.candidate = pressed, .stable = pressed, .since = now};
}

/* A stable release arms the next press, including after boot with KEY held. */
static inline bool bsp_button_debounce_update(bsp_button_debounce_t *state, bool pressed, uint32_t now)
{
    if (pressed != state->candidate) {
        state->candidate = pressed;
        state->since = now;
    }
    if ((uint32_t)(now - state->since) < BSP_USER_BUTTON_DEBOUNCE_MS) return false;
    if (!pressed) state->armed = true;
    if (pressed == state->stable) return false;
    state->stable = pressed;
    if (pressed && state->armed) {
        state->armed = false;
        return true;
    }
    return false;
}
