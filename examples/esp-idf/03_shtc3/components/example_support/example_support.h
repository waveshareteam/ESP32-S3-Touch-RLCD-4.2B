/* SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include "bsp/esp-bsp.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Returns the active screen with the display lock held. Unlock after building the UI. */
lv_obj_t *example_screen_begin(const char *title);
lv_obj_t *example_label(lv_obj_t *parent, const char *text, int x, int y, int width,
                        const lv_font_t *font);
lv_obj_t *example_box(lv_obj_t *parent, int x, int y, int width, int height);
void example_label_update(lv_obj_t *label, const char *text);
void example_nvs_init(void);
#ifdef __cplusplus
}
#endif
