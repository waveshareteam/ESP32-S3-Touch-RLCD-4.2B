/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include "example_support.h"
#include "assets/photos.h"

static lv_obj_t *image, *counter;
static size_t current;

static void show_photo(void)
{
    lv_image_set_src(image, example_photos[current]);
    lv_obj_center(image);
    lv_label_set_text_fmt(counter, "<  %u / %u  >", (unsigned)current + 1, (unsigned)example_photo_count);
    lv_obj_align(counter, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static void swipe(lv_event_t *event)
{
    (void)event;
    lv_indev_t *input = lv_indev_active();
    if (!input || example_photo_count < 2) return;
    lv_dir_t direction = lv_indev_get_gesture_dir(input);
    if (direction == LV_DIR_LEFT) current = (current + 1) % example_photo_count;
    else if (direction == LV_DIR_RIGHT) current = current ? current - 1 : example_photo_count - 1;
    else return;
    // One transition per swipe, including when the finger remains on the panel.
    lv_indev_wait_release(input);
    show_photo();
}

void example_main(void)
{
    lv_obj_t *screen = example_screen_begin(NULL);
    if (!example_photo_count) {
        example_label(screen, "Add images to the photo registry.", 20, 130, 360, &lv_font_montserrat_20);
        bsp_display_unlock();
        return;
    }
    image = lv_image_create(screen);
    lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    counter = example_label(screen, "", 0, 0, 160, &lv_font_montserrat_16);
    lv_obj_set_style_text_align(counter, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(counter, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(counter, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(counter, 6, 0);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, swipe, LV_EVENT_GESTURE, NULL);
    show_photo();
    bsp_display_unlock();
}
