/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/*
 * Port of D:/RLCD/custom/custom.c.
 * GUI Guider still owns the layout; monochrome adaptation and game state live here.
 */
#include "custom.h"
#include <stdio.h>
#ifdef ESP_PLATFORM
#include "esp_random.h"
#endif

static game_2048_state_t game;
static bool paused;
static char number_text[16][12];
static const char *button_map[20];

static uint32_t random_word(void)
{
#ifdef ESP_PLATFORM
    return esp_random();
#else
    return (lv_rand(0, 65535) << 16) | lv_rand(0, 65535);
#endif
}

const game_2048_state_t *game_2048_get_state(void) { return &game; }
void custom_set_paused(bool value) { paused = value; }

static bool dark_tile(uint32_t value)
{
    return (value >= 8 && value <= 64) || value >= 1024;
}

static void draw_tile_event(lv_event_t *event)
{
    lv_draw_task_t *task = lv_event_get_draw_task(event);
    lv_draw_dsc_base_t *base = lv_draw_task_get_draw_dsc(task);
    if (base->part != LV_PART_ITEMS || base->id1 >= 16) return;
    const uint32_t value = game.cells[base->id1 / 4][base->id1 % 4];
    const bool dark = dark_tile(value);
    lv_draw_fill_dsc_t *fill = lv_draw_task_get_fill_dsc(task);
    if (fill != NULL) {
        fill->color = dark ? lv_color_black() : lv_color_white();
        fill->opa = LV_OPA_COVER;
        fill->radius = 4;
    }
    lv_draw_label_dsc_t *label = lv_draw_task_get_label_dsc(task);
    if (label != NULL) label->color = dark ? lv_color_white() : lv_color_black();
    lv_draw_border_dsc_t *border = lv_draw_task_get_border_dsc(task);
    if (border != NULL) {
        border->color = lv_color_black();
        border->opa = LV_OPA_COVER;
        border->width = value == 0 ? 1 : 2;
        border->radius = 4;
    }
}

static void monochrome_object(lv_obj_t *obj, bool dark)
{
    lv_obj_set_style_bg_color(obj, dark ? lv_color_black() : lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(obj, dark ? lv_color_white() : lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_black(), LV_PART_MAIN);
}

static void style_button(lv_obj_t *button, lv_obj_t *label)
{
    const int32_t x = lv_obj_get_style_x(button, LV_PART_MAIN);
    const int32_t y = lv_obj_get_style_y(button, LV_PART_MAIN);
    const int32_t width = lv_obj_get_style_width(button, LV_PART_MAIN);
    const int32_t height = lv_obj_get_style_height(button, LV_PART_MAIN);
    // Remove the theme's animated pressed styles; black/white feedback must be immediate.
    lv_obj_remove_style_all(button);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
    lv_obj_set_style_text_align(button, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    monochrome_object(button, true);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 4, LV_PART_MAIN);
    lv_obj_set_style_text_font(button, &lv_font_Alatsi_Regular_21, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_white(), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(button, lv_color_black(), LV_STATE_PRESSED);
    // GUI Guider labels inherit the button text style.
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_Alatsi_Regular_21, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_black(), LV_STATE_PRESSED);
    lv_obj_add_flag(label, LV_OBJ_FLAG_STATE_TRICKLE);
    lv_obj_add_flag(button, LV_OBJ_FLAG_STATE_TRICKLE);
    lv_obj_remove_flag(button, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

static void apply_monochrome_theme(lv_ui *ui)
{
    monochrome_object(ui->Game2048, false);
    lv_obj_set_style_pad_all(ui->Game2048, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->Game2048, 0, LV_PART_MAIN);
    lv_obj_remove_flag(ui->Game2048, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *board = ui->Game2048_btnm_2048;
    // Leave the shared 48px header clear, with 8px above and below the board.
    lv_obj_set_pos(board, 152, 56);
    lv_obj_set_size(board, 236, 236);
    monochrome_object(board, false);
    lv_obj_set_style_border_width(board, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(board, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(board, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_row(board, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_column(board, 4, LV_PART_MAIN);
    lv_obj_set_style_text_font(board, &lv_font_Alatsi_Regular_21, LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(board, 0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(board, 2, LV_PART_ITEMS);
    lv_obj_set_style_border_color(board, lv_color_black(), LV_PART_ITEMS);
    lv_obj_set_style_radius(board, 4, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(board, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(board, lv_color_black(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(board, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(board, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(board, lv_color_black(), LV_PART_ITEMS | LV_STATE_PRESSED);

    monochrome_object(ui->Game2048_label_2048, false);
    lv_obj_add_flag(ui->Game2048_label_2048, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(ui->Game2048_btn_new_game, 12, 56);
    lv_obj_set_size(ui->Game2048_btn_new_game, 128, 68);
    lv_label_set_text(ui->Game2048_btn_new_game_label, "New\nGame");
    style_button(ui->Game2048_btn_new_game, ui->Game2048_btn_new_game_label);
    lv_obj_set_style_radius(ui->Game2048_btn_new_game, 6, LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->Game2048_btn_new_game, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->Game2048_btn_new_game_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(ui->Game2048_btn_new_game_label, 0, LV_PART_MAIN);
    lv_obj_set_style_text_align(ui->Game2048_btn_new_game_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(ui->Game2048_btn_new_game_label);

    lv_obj_t *titles[] = {ui->Game2048_label_score_title, ui->Game2048_label_best_title};
    lv_obj_t *values[] = {ui->Game2048_label_score, ui->Game2048_label_best};
    for (int i = 0; i < 2; ++i) {
        const int y = 136 + i * 80;
        monochrome_object(titles[i], false);
        lv_obj_set_pos(titles[i], 12, y);
        lv_obj_set_size(titles[i], 128, 32);
        lv_obj_set_style_bg_opa(titles[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_text_font(titles[i], &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_pad_all(titles[i], 0, LV_PART_MAIN);
        lv_label_set_long_mode(titles[i], LV_LABEL_LONG_DOT);
        monochrome_object(values[i], false);
        lv_obj_set_pos(values[i], 12, y + 36);
        lv_obj_set_size(values[i], 128, 40);
        lv_obj_set_style_text_font(values[i], &lv_font_montserrat_32, LV_PART_MAIN);
        lv_label_set_long_mode(values[i], LV_LABEL_LONG_DOT);
    }

    monochrome_object(ui->Game2048_cont_msgbox, false);
    lv_obj_set_pos(ui->Game2048_cont_msgbox, 0, 0);
    lv_obj_set_size(ui->Game2048_cont_msgbox, 400, 300);
    lv_obj_set_style_bg_opa(ui->Game2048_cont_msgbox, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->Game2048_cont_msgbox, 0, LV_PART_MAIN);
    lv_obj_remove_flag(ui->Game2048_cont_msgbox, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
    monochrome_object(ui->Game2048_label_warning, false);
    lv_obj_set_pos(ui->Game2048_label_warning, 12, 62);
    lv_obj_set_size(ui->Game2048_label_warning, 376, 42);
    monochrome_object(ui->Game2048_label_results_title, false);
    monochrome_object(ui->Game2048_label_results, false);
    lv_obj_set_pos(ui->Game2048_btn_again, 142, 237);
    lv_obj_set_size(ui->Game2048_btn_again, 116, 40);
    style_button(ui->Game2048_btn_again, ui->Game2048_btn_again_label);
}

static void refresh(lv_ui *ui)
{
    unsigned index = 0;
    for (unsigned i = 0; i < 16; ++i) {
        const uint32_t value = game.cells[i / 4][i % 4];
        if (value) snprintf(number_text[i], sizeof(number_text[i]), "%" LV_PRIu32, value);
        else { number_text[i][0] = ' '; number_text[i][1] = '\0'; }
        button_map[index++] = number_text[i];
        if (i % 4 == 3 && i != 15) button_map[index++] = "\n";
    }
    button_map[index] = "";
    lv_buttonmatrix_set_map(ui->Game2048_btnm_2048, button_map);
    lv_label_set_text_fmt(ui->Game2048_label_score, "%" LV_PRIu32, game.score);
    lv_label_set_text_fmt(ui->Game2048_label_best, "%" LV_PRIu32, game.best);

    if (game.won || game.over) {
        lv_label_set_text(ui->Game2048_label_warning, game.won ? "You Win!" : "Game Over");
        lv_label_set_text_fmt(ui->Game2048_label_results, "%" LV_PRIu32 "\n%" LV_PRIu32 "\n%" LV_PRIu32,
                             game.moves, game.score, game.best);
        lv_obj_remove_flag(ui->Game2048_cont_msgbox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui->Game2048_cont_msgbox);
    } else {
        lv_obj_add_flag(ui->Game2048_cont_msgbox, LV_OBJ_FLAG_HIDDEN);
    }
}

void new_game(lv_ui *ui)
{
    if (paused) return;
    game_2048_reset(&game);
    game_2048_spawn(&game, random_word(), random_word());
    game_2048_spawn(&game, random_word(), random_word());
    refresh(ui);
}

void custom_init(lv_ui *ui)
{
    paused = false;
    apply_monochrome_theme(ui);
    lv_obj_add_event_cb(ui->Game2048_btnm_2048, draw_tile_event, LV_EVENT_DRAW_TASK_ADDED, ui);
    lv_obj_add_flag(ui->Game2048_btnm_2048, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    new_game(ui);
}

void movement_check(lv_ui *ui, gg_2048_move_direction direction)
{
    if (paused || game.won || game.over) return;
    if (game_2048_slide(&game, direction)) {
        game_2048_spawn(&game, random_word(), random_word());
    }
    refresh(ui);
}
