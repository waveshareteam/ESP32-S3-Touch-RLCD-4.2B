#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*settings_close_cb_t)(void *context);
typedef enum {
    SETTINGS_HOME, SETTINGS_WIFI_SCAN, SETTINGS_QR, SETTINGS_SD, SETTINGS_ABOUT
} settings_page_t;
bool settings_ui_init(lv_obj_t *parent, int32_t x, int32_t y, settings_close_cb_t close, void *context);
void settings_ui_deinit(void);
void settings_ui_pause(bool paused);
/* true means a child page consumed Back; false means the APP may close. */
bool settings_ui_back(void);
#ifdef __cplusplus
}
#endif
