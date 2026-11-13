#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*weather_back_cb_t)(void *context);
bool init_weather_ui_screen(lv_obj_t *parent,int32_t x,int32_t y,weather_back_cb_t back,void *context);
void deinit_weather_ui(void);
void pause_weather_ui(bool paused);
#ifdef __cplusplus
}
#endif
