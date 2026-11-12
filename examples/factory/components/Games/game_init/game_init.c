#include "game_init.h"
#include "custom.h"
#include "rlcd_app_header.h"
#include <string.h>

static lv_ui game_ui;
static game_back_cb_t back_callback;
static void *back_context;

static void on_ui_delete(lv_event_t *event)
{
    if (lv_event_get_target(event) == game_ui.Game2048) {
        memset(&game_ui, 0, sizeof(game_ui));
        back_callback = NULL;
        back_context = NULL;
    }
}

static void back_event(lv_event_t *event)
{
    (void)event;
    if (back_callback != NULL) back_callback(back_context);
}

bool init_game_ui_screen(lv_obj_t *parent, int32_t x, int32_t y, game_back_cb_t on_back, void *context)
{
    if (parent == NULL) return false;
    deinit_game_ui_screen();
    back_callback = on_back;
    back_context = context;
    setup_scr_Game2048(&game_ui, parent);
    lv_obj_set_user_data(game_ui.Game2048, &game_ui);
    lv_obj_set_pos(game_ui.Game2048, x, y);
    lv_obj_add_event_cb(game_ui.Game2048, on_ui_delete, LV_EVENT_DELETE, NULL);
    custom_init(&game_ui);
    rlcd_app_header_create(game_ui.Game2048, "2048", &rlcd_header_2048, back_event, NULL);
    rlcd_app_header_create(game_ui.Game2048_cont_msgbox, "2048", &rlcd_header_2048, back_event, NULL);
    return true;
}

void deinit_game_ui_screen(void)
{
    if (game_ui.Game2048 != NULL) {
        lv_obj_delete(game_ui.Game2048);
    }
}

void pause_game_ui_screen(bool paused)
{
    custom_set_paused(paused);
}
