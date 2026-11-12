#pragma once
#include "lvgl.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*music_back_fn)(void *ctx);
bool music_ui_init(lv_obj_t *parent, int x, int y, music_back_fn back, void *ctx);
void music_ui_deinit(void);
void music_ui_pause(bool paused);
#ifdef __cplusplus
}
#endif
