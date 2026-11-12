#pragma once
#include "gui_guider.h"

typedef void (*game_back_cb_t)(void *context);

#ifdef __cplusplus
extern "C" {
#endif

bool init_game_ui_screen(lv_obj_t *parent, int32_t x, int32_t y, game_back_cb_t on_back, void *context);
void deinit_game_ui_screen(void);
void pause_game_ui_screen(bool paused);

#ifdef __cplusplus
}
#endif
