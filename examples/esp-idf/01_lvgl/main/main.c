/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include "example_support.h"

static lv_obj_t *counter_label;
static unsigned counter;

static void count_click(lv_event_t *event)
{
    (void)event;
    lv_label_set_text_fmt(counter_label, "Touch count: %u", ++counter);
}

void app_main(void)
{
    lv_obj_t *screen = example_screen_begin("LVGL 9 / RLCD");
    lv_obj_t *subtitle = example_label(screen, "400 x 300  |  MONOCHROME", 20, 53, 360, &lv_font_montserrat_14);
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *card = example_box(screen, 20, 101, 360, 156);
    counter_label = example_label(card, "Touch count: 0", 0, 24, 360, &lv_font_montserrat_24);
    lv_obj_set_style_text_align(counter_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *button = lv_button_create(card);
    lv_obj_remove_style_all(button);
    lv_obj_set_size(button, 240, 58);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(button, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(button, lv_color_white(), 0);
    lv_obj_set_style_radius(button, 10, 0);
    lv_obj_set_style_outline_color(button, lv_color_black(), LV_STATE_PRESSED);
    lv_obj_set_style_outline_width(button, 2, LV_STATE_PRESSED);
    lv_obj_set_style_outline_pad(button, 3, LV_STATE_PRESSED);
    lv_obj_t *caption = lv_label_create(button);
    lv_obj_set_style_text_font(caption, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(caption, lv_color_white(), 0);
    lv_obj_set_style_text_opa(caption, LV_OPA_COVER, 0);
    lv_obj_set_style_text_letter_space(caption, 1, 0);
    lv_label_set_text(caption, "Tap to count");
    lv_obj_center(caption);
    lv_obj_add_event_cb(button, count_click, LV_EVENT_CLICKED, NULL);
    bsp_display_unlock();
}
