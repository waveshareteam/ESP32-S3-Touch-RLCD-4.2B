#pragma once
#include "lvgl.h"
#include "weather_model.h"
#ifdef __cplusplus
extern "C" {
#endif
LV_IMAGE_DECLARE(image_weather_mono);
const lv_image_dsc_t *weather_icon_get(weather_condition_t condition);
#ifdef __cplusplus
}
#endif
