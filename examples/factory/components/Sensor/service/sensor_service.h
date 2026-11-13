#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SENSOR_WAITING, SENSOR_READY, SENSOR_UNAVAILABLE
} sensor_state_t;

typedef struct {
    sensor_state_t environment_state, clock_state;
    float temperature, humidity;
    int year, month, day, hour, minute, second;
} sensor_snapshot_t;
#ifdef __cplusplus
extern "C" {
#endif
/* Called by the GUI thread. Creation is lazy; all hardware access is on the worker. */
bool sensor_service_start(void);
void sensor_service_set_active(bool active);
void sensor_service_get_snapshot(sensor_snapshot_t *out);
#ifdef __cplusplus
}
#endif
