#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RLCD_APP_HEADER_HEIGHT 48

typedef struct {
    lv_obj_t *root;
    lv_obj_t *title;
    lv_obj_t *back;
} rlcd_app_header_t;

LV_IMAGE_DECLARE(rlcd_header_weather);
LV_IMAGE_DECLARE(rlcd_header_music);
LV_IMAGE_DECLARE(rlcd_header_2048);
LV_IMAGE_DECLARE(rlcd_header_photos);
LV_IMAGE_DECLARE(rlcd_header_sensor);
LV_IMAGE_DECLARE(rlcd_header_settings);
LV_IMAGE_DECLARE(rlcd_header_button_test);
LV_IMAGE_DECLARE(rlcd_header_mic_test);

rlcd_app_header_t rlcd_app_header_create(lv_obj_t *parent, const char *title,
                                       const lv_image_dsc_t *icon,
                                       lv_event_cb_t on_back, void *context);

#ifdef __cplusplus
}
#endif
