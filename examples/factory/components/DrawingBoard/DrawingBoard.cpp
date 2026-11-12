/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adapted from ESP32-C5-Touch-LCD-2.8/08_bookesia.
 * Brookesia touch application for a 400x300 monochrome display.
 */
#include "DrawingBoard.hpp"
#include "src/misc/cache/instance/lv_image_cache.h"

#include <cstring>
#include <new>

LV_IMAGE_DECLARE(image_draw);

namespace {

constexpr int32_t kWidth = 400;
constexpr int32_t kHeight = 300;
constexpr int32_t kBrushWidth = 10;

// Fixed storage: 15000 pixel bytes, a two-color palette and alignment padding.
alignas(LV_DRAW_BUF_ALIGN) uint8_t s_pixels[LV_DRAW_BUF_SIZE(kWidth, kHeight, LV_COLOR_FORMAT_I1)];
lv_draw_buf_t s_draw_buf;
lv_obj_t *s_canvas = nullptr;
lv_point_t s_previous;
bool s_stroke_active = false;

void clear_canvas()
{
    s_stroke_active = false;
    // I1 palette index 1 is white. Keep the palette itself unchanged.
    std::memset(lv_draw_buf_goto_xy(&s_draw_buf, 0, 0), 0xff,
                s_draw_buf.header.stride * kHeight);
    lv_draw_buf_flush_cache(&s_draw_buf, nullptr);
    lv_image_cache_drop(&s_draw_buf);
    lv_obj_invalidate(s_canvas);
}

void draw_to(const lv_point_t &point)
{
    if (s_stroke_active && point.x == s_previous.x && point.y == s_previous.y) {
        return;
    }

    lv_layer_t layer;
    lv_canvas_init_layer(s_canvas, &layer);

    if (s_stroke_active) {
        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.color = lv_color_black();
        line.width = kBrushWidth;
        line.round_start = true;
        line.round_end = true;
        line.p1.x = s_previous.x;
        line.p1.y = s_previous.y;
        line.p2.x = point.x;
        line.p2.y = point.y;
        lv_draw_line(&layer, &line);
    } else {
        lv_draw_rect_dsc_t dot;
        lv_draw_rect_dsc_init(&dot);
        dot.bg_color = lv_color_black();
        dot.bg_opa = LV_OPA_COVER;
        dot.radius = LV_RADIUS_CIRCLE;
        const lv_area_t area = {
            point.x - kBrushWidth / 2,
            point.y - kBrushWidth / 2,
            point.x - kBrushWidth / 2 + kBrushWidth - 1,
            point.y - kBrushWidth / 2 + kBrushWidth - 1,
        };
        lv_draw_rect(&layer, &dot, &area);
    }

    lv_canvas_finish_layer(s_canvas, &layer);
    lv_image_cache_drop(&s_draw_buf);
    s_previous = point;
    s_stroke_active = true;
}

void touch_event_cb(lv_event_t *event)
{
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_DELETE) {
        lv_image_cache_drop(&s_draw_buf);
        s_canvas = nullptr;
        s_stroke_active = false;
        return;
    }
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        // A new press starts a new stroke, never joining separate touches.
        s_stroke_active = false;
        return;
    }
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) {
        return;
    }

    lv_indev_t *indev = lv_indev_active();
    if (indev == nullptr) {
        return;
    }
    if (code == LV_EVENT_PRESSED) {
        s_stroke_active = false;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    lv_area_t bounds;
    lv_obj_get_coords(s_canvas, &bounds);
    point.x -= bounds.x1;
    point.y -= bounds.y1;
    if (point.x < 0 || point.x >= kWidth || point.y < 0 || point.y >= kHeight) {
        s_stroke_active = false;
        return;
    }

    draw_to(point);
}

void clear_event_cb(lv_event_t *event)
{
    (void)event;
    clear_canvas();
}

} // namespace

static lv_obj_t *drawing_board_create(lv_obj_t *parent)
{
    if (parent == nullptr) {
        return nullptr;
    }
    if (s_canvas != nullptr) {
        return s_canvas;
    }
    if (lv_draw_buf_init(&s_draw_buf, kWidth, kHeight, LV_COLOR_FORMAT_I1,
                         LV_DRAW_BUF_STRIDE(kWidth, LV_COLOR_FORMAT_I1),
                         s_pixels, sizeof(s_pixels)) != LV_RESULT_OK) {
        return nullptr;
    }
    lv_draw_buf_set_flag(&s_draw_buf, LV_IMAGE_FLAGS_MODIFIABLE);

    s_canvas = lv_canvas_create(parent);
    lv_obj_remove_style_all(s_canvas);
    lv_canvas_set_draw_buf(s_canvas, &s_draw_buf);
    lv_canvas_set_palette(s_canvas, 0, lv_color32_make(0, 0, 0, 255));
    lv_canvas_set_palette(s_canvas, 1, lv_color32_make(255, 255, 255, 255));
    lv_obj_set_pos(s_canvas, 0, 0);
    lv_obj_add_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_canvas, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_add_event_cb(s_canvas, touch_event_cb, LV_EVENT_ALL, nullptr);
    clear_canvas();

    // Keep the title readable over strokes, without intercepting canvas touches.
    lv_obj_t *title = lv_label_create(s_canvas);
    lv_label_set_text(title, "Touch Test");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(title, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(title, 4, LV_PART_MAIN);
    lv_obj_remove_flag(title, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

    // Floating button keeps the canvas at the full 400x300 resolution.
    lv_obj_t *clear_button = lv_button_create(s_canvas);
    lv_obj_set_size(clear_button, 68, 32);
    lv_obj_align(clear_button, LV_ALIGN_TOP_RIGHT, -6, 6);
    lv_obj_set_style_bg_color(clear_button, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(clear_button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(clear_button, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(clear_button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(clear_button, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(clear_button, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(clear_button, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(clear_button, lv_color_black(), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(clear_button, lv_color_white(), LV_STATE_PRESSED);
    lv_obj_remove_flag(clear_button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(clear_button, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(clear_button, clear_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *label = lv_label_create(clear_button);
    lv_label_set_text(label, "Clear");
    lv_obj_center(label);

    return s_canvas;
}

namespace esp_brookesia::apps {

DrawingBoard *DrawingBoard::_instance = nullptr;

DrawingBoard *DrawingBoard::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new (std::nothrow) DrawingBoard(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

DrawingBoard::DrawingBoard(bool use_status_bar, bool use_navigation_bar)
    : App("Drawing", &image_draw, true, use_status_bar, use_navigation_bar)
{
}

DrawingBoard::~DrawingBoard()
{
    _instance = nullptr;
}

bool DrawingBoard::run()
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    const lv_area_t area = getVisualArea();
    if (lv_area_get_width(&area) != kWidth || lv_area_get_height(&area) != kHeight) {
        return false;
    }
    lv_obj_t *canvas = drawing_board_create(screen);
    if (canvas == nullptr) {
        return false;
    }
    lv_obj_set_pos(canvas, area.x1, area.y1);

    // Keep the Back button and support Brookesia navigation gestures for returning to the desktop.
    lv_obj_t *button = lv_button_create(canvas);
    lv_obj_set_size(button, 68, 32);
    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 6, 6);
    lv_obj_set_style_bg_color(button, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(button, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_black(), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(button, lv_color_white(), LV_STATE_PRESSED);
    lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(button, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(button, back_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "Back");
    lv_obj_center(label);
    return true;
}

void DrawingBoard::back_event_cb(lv_event_t *event)
{
    auto *app = static_cast<DrawingBoard *>(lv_event_get_user_data(event));
    app->back();
}

bool DrawingBoard::back()
{
    s_stroke_active = false;
    return notifyCoreClosed();
}

bool DrawingBoard::close()
{
    s_stroke_active = false;
    // Brookesia cleans up the app's default screen; the canvas DELETE callback clears widget pointers.
    return true;
}

bool DrawingBoard::init() { return true; }
bool DrawingBoard::deinit() { return true; }
bool DrawingBoard::pause() { s_stroke_active = false; return true; }
bool DrawingBoard::resume() { s_stroke_active = false; return true; }

} // namespace esp_brookesia::apps
