#include "rlcd_app_header.h"

rlcd_app_header_t rlcd_app_header_create(lv_obj_t *parent, const char *title,
                                       const lv_image_dsc_t *icon,
                                       lv_event_cb_t on_back, void *context)
{
    rlcd_app_header_t header = {0};
    header.root = lv_obj_create(parent);
    lv_obj_remove_style_all(header.root);
    lv_obj_set_pos(header.root, 0, 0);
    lv_obj_set_size(header.root, 400, RLCD_APP_HEADER_HEIGHT);
    lv_obj_remove_flag(header.root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_color(header.root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(header.root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(header.root, lv_color_white(), 0);
    lv_obj_set_style_text_letter_space(header.root, 0, 0);

    lv_obj_t *image = lv_image_create(header.root);
    lv_image_set_src(image, icon);
    lv_obj_set_pos(image, 12, 8);
    lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);

    header.title = lv_label_create(header.root);
    lv_obj_remove_style_all(header.title);
    lv_obj_set_pos(header.title, 51, 13);
    lv_obj_set_size(header.title, 257, lv_font_montserrat_20.line_height);
    lv_obj_set_style_text_font(header.title, &lv_font_montserrat_20, 0);
    lv_label_set_long_mode(header.title, LV_LABEL_LONG_DOT);
    lv_label_set_text(header.title, title);

    header.back = lv_button_create(header.root);
    lv_obj_remove_style_all(header.back);
    lv_obj_set_pos(header.back, 323, 7);
    lv_obj_set_size(header.back, 65, 34);
    lv_obj_remove_flag(header.back, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_opa(header.back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_font(header.back, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(header.back, lv_color_white(), 0);
    lv_obj_t *caption = lv_label_create(header.back);
    lv_obj_remove_style_all(caption);
    lv_label_set_text(caption, LV_SYMBOL_LEFT);
    lv_obj_center(caption);
    lv_obj_add_event_cb(header.back, on_back, LV_EVENT_CLICKED, context);
    return header;
}
