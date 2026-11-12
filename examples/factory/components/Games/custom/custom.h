#pragma once
#include "gui_guider.h"
#include "game_2048.h"

#ifdef __cplusplus
extern "C" {
#endif

void custom_init(lv_ui *ui);
void custom_set_paused(bool paused);
void new_game(lv_ui *ui);
void movement_check(lv_ui *ui, gg_2048_move_direction direction);
/* Exposed for host regression tests and future app extensions. */
const game_2048_state_t *game_2048_get_state(void);

#ifdef __cplusplus
}
#endif
