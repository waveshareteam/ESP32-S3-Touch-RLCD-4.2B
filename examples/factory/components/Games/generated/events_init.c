/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include "custom.h"

static void board_event_handler(lv_event_t *event)
{
    lv_ui *ui = lv_event_get_user_data(event);
    if (!lv_obj_has_flag(ui->Game2048_cont_msgbox, LV_OBJ_FLAG_HIDDEN)) return;
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) return;

    gg_2048_move_direction direction;
    switch (lv_indev_get_gesture_dir(indev)) {
    case LV_DIR_LEFT: direction = GG_2048_MOVE_LEFT; break;
    case LV_DIR_RIGHT: direction = GG_2048_MOVE_RIGHT; break;
    case LV_DIR_TOP: direction = GG_2048_MOVE_UP; break;
    case LV_DIR_BOTTOM: direction = GG_2048_MOVE_DOWN; break;
    default: return;
    }
    // One move per swipe, as in the GUI Guider example.
    lv_indev_wait_release(indev);
    movement_check(ui, direction);
}

static void new_game_event_handler(lv_event_t *event)
{
    new_game(lv_event_get_user_data(event));
}

void events_init_Game2048(lv_ui *ui)
{
    // Only swipes starting on the board move tiles. Screen-edge navigation remains available.
    lv_obj_add_flag(ui->Game2048_btnm_2048, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_remove_flag(ui->Game2048_btnm_2048, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui->Game2048_btnm_2048, board_event_handler, LV_EVENT_GESTURE, ui);
    lv_obj_add_event_cb(ui->Game2048_btn_new_game, new_game_event_handler, LV_EVENT_CLICKED, ui);
    lv_obj_add_event_cb(ui->Game2048_btn_again, new_game_event_handler, LV_EVENT_CLICKED, ui);
}

void events_init(lv_ui *ui) { (void)ui; }
