/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 * One-second color cycling adapted from the C5-3.5 Display Test. */
#include "DisplayTest.hpp"
#include "esp_log.h"
#include <new>

LV_IMAGE_DECLARE(img_app_display_test);
namespace {
constexpr uint32_t COLOR_PERIOD_MS = 1000, DOUBLE_TAP_MS = 400;
esp_brookesia::systems::phone::App::Config fullscreen_config()
{
    auto config = esp_brookesia::systems::phone::App::Config::SIMPLE_CONSTRUCTOR(
        &img_app_display_test, false, false);
    config.flags.enable_navigation_gesture = false;
    return config;
}
}
namespace esp_brookesia::apps {
DisplayTest *DisplayTest::instance_;
DisplayTest *DisplayTest::requestInstance()
{
    if (!instance_) instance_ = new (std::nothrow) DisplayTest;
    return instance_;
}
DisplayTest::DisplayTest()
    : App(systems::base::App::Config::SIMPLE_CONSTRUCTOR("Display Test", &img_app_display_test, true),
          fullscreen_config()) {}
DisplayTest::~DisplayTest() { close(); instance_ = nullptr; }
bool DisplayTest::run()
{
    close();
    const auto area = getVisualArea();
    white_ = false; tap_pending_ = false;
    root_ = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(root_);
    lv_obj_set_pos(root_, area.x1, area.y1);
    lv_obj_set_size(root_, lv_area_get_width(&area), lv_area_get_height(&area));
    lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN | LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(root_, touchEvent, LV_EVENT_SHORT_CLICKED, this);
    timer_ = lv_timer_create(cycle, COLOR_PERIOD_MS, this);
    if (!timer_) { close(); return false; }
    ESP_LOGI("display_test", "Black/white, 1 s each; LVGL double tap anywhere to exit");
    return true;
}
void DisplayTest::cycle(lv_timer_t *timer)
{
    auto *app = static_cast<DisplayTest *>(lv_timer_get_user_data(timer));
    app->white_ = !app->white_;
    lv_obj_set_style_bg_color(app->root_, app->white_ ? lv_color_white() : lv_color_black(), 0);
}
void DisplayTest::touchEvent(lv_event_t *event)
{
    auto *app = static_cast<DisplayTest *>(lv_event_get_user_data(event));
    // Count only this APP's LVGL clicks, excluding the desktop launch tap.
    // The full screen is one target; neither color changes nor tap position reset it.
    if (app->tap_pending_ && lv_tick_elaps(app->tap_tick_) <= DOUBLE_TAP_MS) {
        app->tap_pending_ = false;
        lv_timer_pause(app->timer_);
        ESP_LOGI("display_test", "Double tap: returning to desktop");
        if (!app->back()) lv_timer_resume(app->timer_);
        return;
    }
    app->tap_pending_ = true;
    app->tap_tick_ = lv_tick_get();
}
bool DisplayTest::pause()
{
    tap_pending_ = false;
    if (timer_) lv_timer_pause(timer_);
    return true;
}
bool DisplayTest::resume()
{
    tap_pending_ = false;
    if (timer_) { lv_timer_reset(timer_); lv_timer_resume(timer_); }
    return true;
}
bool DisplayTest::close()
{
    if (timer_) { lv_timer_delete(timer_); timer_ = nullptr; }
    if (root_) { lv_obj_delete(root_); root_ = nullptr; }
    tap_pending_ = false;
    return true;
}
}
