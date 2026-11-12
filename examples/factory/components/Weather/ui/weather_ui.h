#pragma once
#include "lvgl.h"
#include "weather_service.h"
#include "wifi_manager.h"

typedef void (*weather_action_cb_t)(void *context);
typedef struct {
    lv_obj_t *screen;
    lv_obj_t *screen_weather_img;
    lv_obj_t *screen_diqu_lab;
    lv_obj_t *screen_data_lab;
    lv_obj_t *screen_weather_lab;
    lv_obj_t *screen_wendu_lab;
    lv_obj_t *screen_shidu_lab;
    lv_obj_t *screen_fengsu_lab;
    lv_obj_t *screen_fengxiang_lab;
    lv_obj_t *temperature_unit;
    lv_obj_t *status_label;
    lv_obj_t *back_button;
    weather_action_cb_t back, refresh;
    void *context;
} weather_ui_t;

void weather_ui_create(weather_ui_t *ui, lv_obj_t *parent, int32_t x, int32_t y,
                        weather_action_cb_t back, weather_action_cb_t refresh, void *context);
void weather_ui_update(weather_ui_t *ui, const weather_snapshot_t *weather,
                        const wifi_manager_snapshot_t *network);
