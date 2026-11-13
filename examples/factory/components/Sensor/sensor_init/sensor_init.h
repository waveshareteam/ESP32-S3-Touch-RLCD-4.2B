#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*sensor_back_cb_t)(void *context);
typedef struct {
    lv_obj_t *root, *back, *temperature, *humidity, *date, *time;
    lv_obj_t *temperature_unit, *humidity_unit;
    sensor_back_cb_t on_back;
    void *context;
} sensor_ui_t;
bool init_sensor_ui_screen(lv_obj_t *parent, int32_t x, int32_t y, sensor_back_cb_t back, void *context);
void deinit_sensor_ui(void);
void pause_sensor_ui(bool paused);
#ifdef __cplusplus
}
#endif
