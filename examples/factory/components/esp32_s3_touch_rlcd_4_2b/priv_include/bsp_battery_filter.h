#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BSP_BATTERY_STALE_US (15LL * 1000000)
#define BSP_BATTERY_PERCENT_STEP_US (30LL * 1000000)
#define BSP_BATTERY_HYSTERESIS_MV 12

typedef struct {
    bool initialized;
    int voltage_q8;
    int percent;
    int64_t updated_us;
    int64_t percent_changed_us;
} bsp_battery_filter_t;

static inline int bsp_battery_estimate_percent(int voltage_mv)
{
    /* Keep the factory voltage scale; this is not a coulomb counter. */
    if (voltage_mv <= 3000) return 0;
    if (voltage_mv >= 4120) return 100;
    return (voltage_mv - 3000) * 100 / 1120;
}

static inline int bsp_battery_filter_voltage(const bsp_battery_filter_t *state)
{
    return (state->voltage_q8 + 128) / 256;
}

/* Called with a valid trimmed mean about every 2 seconds. The fixed-point EMA
 * retains sub-millivolt changes. USB/load changes still affect the estimate;
 * limit their display rate without pretending to detect charging direction. */
static inline void bsp_battery_filter_update(bsp_battery_filter_t *state, int voltage_mv, int64_t now_us)
{
    if (!state->initialized || now_us - state->updated_us > BSP_BATTERY_STALE_US) {
        *state = (bsp_battery_filter_t){
            .initialized = true,
            .voltage_q8 = voltage_mv * 256,
            .percent = bsp_battery_estimate_percent(voltage_mv),
            .updated_us = now_us,
            .percent_changed_us = now_us,
        };
        return;
    }

    state->voltage_q8 += (voltage_mv * 256 - state->voltage_q8) / 8;
    state->updated_us = now_us;
    const int filtered_mv = bsp_battery_filter_voltage(state);
    const int target = bsp_battery_estimate_percent(filtered_mv);

    /* Do not hold a high percentage for minutes when voltage is already low.
     * This is display behavior, not a battery protection/shutdown threshold. */
    if (filtered_mv <= 3300 && target < state->percent) {
        state->percent = target;
        state->percent_changed_us = now_us;
        return;
    }
    if (now_us - state->percent_changed_us < BSP_BATTERY_PERCENT_STEP_US) return;

    if (bsp_battery_estimate_percent(filtered_mv - BSP_BATTERY_HYSTERESIS_MV) > state->percent) {
        ++state->percent;
        state->percent_changed_us = now_us;
    } else if (bsp_battery_estimate_percent(filtered_mv + BSP_BATTERY_HYSTERESIS_MV) < state->percent) {
        --state->percent;
        state->percent_changed_us = now_us;
    }
}
