/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 * Press/release sequence adapted from the C5-3.5 Button Test. */
#include "ButtonTest.hpp"
#include "rlcd_app_header.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>
#include <new>

LV_IMAGE_DECLARE(img_app_button_test);

namespace {
constexpr unsigned SAMPLE_MS = 20, STABLE_SAMPLES = 3, MAX_COUNT = 9999;
lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, int width, const lv_font_t *font)
{
    auto *obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y); lv_obj_set_width(obj, width);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_label_set_text(obj, text);
    return obj;
}
void text(lv_obj_t *obj, const char *value)
{
    if (std::strcmp(lv_label_get_text(obj), value)) lv_label_set_text(obj, value);
}
lv_obj_t *button(lv_obj_t *parent, const char *symbol, int x, int y)
{
    auto *obj = lv_button_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, 34, 34);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_black(), 0);
    lv_obj_set_style_radius(obj, 8, 0);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    lv_obj_set_style_bg_color(obj, lv_color_black(), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(obj, lv_color_white(), LV_STATE_PRESSED);
    auto *caption = lv_label_create(obj);
    lv_label_set_text(caption, symbol); lv_obj_center(caption);
    return obj;
}
}

namespace esp_brookesia::apps {
ButtonTest *ButtonTest::instance_;
ButtonTest *ButtonTest::requestInstance()
{
    if (!instance_) instance_ = new (std::nothrow) ButtonTest;
    return instance_;
}
ButtonTest::ButtonTest() : App("Button Test", &img_app_button_test, true, false, false) {}
ButtonTest::~ButtonTest() { close(); instance_ = nullptr; }

bool ButtonTest::Sequence::update(int raw)
{
    if (raw < 0) { *this = {}; return false; }
    if (raw != candidate) { candidate = raw; samples = 1; }
    else if (samples < STABLE_SAMPLES) ++samples;
    if (samples < STABLE_SAMPLES) return false;
    state = raw;
    if (phase == InitialRelease && !raw) phase = Press;
    else if (phase == Press && raw) phase = FinalRelease;
    else if (phase == FinalRelease && !raw) { phase = Press; return true; }
    return false;
}
ButtonTest::Card ButtonTest::createCard(const char *name, const char *signal, int x)
{
    Card card;
    auto *panel = lv_obj_create(root_);
    lv_obj_remove_style_all(panel);
    lv_obj_set_pos(panel, x, 66); lv_obj_set_size(panel, 176, 158);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, lv_color_black(), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    label(panel, name, 12, 12, 150, &lv_font_montserrat_20);
    label(panel, signal, 12, 41, 150, &lv_font_montserrat_12);
    card.state = label(panel, "WAITING", 12, 67, 152, &lv_font_montserrat_16);
    lv_obj_set_height(card.state, 30);
    lv_obj_set_style_pad_top(card.state, 5, 0);
    lv_obj_set_style_text_align(card.state, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(card.state, LV_OPA_COVER, 0);
    card.count = label(panel, "Presses: 0", 12, 109, 150, &lv_font_montserrat_16);
    card.result = label(panel, "NOT TESTED", 12, 133, 150, &lv_font_montserrat_12);
    return card;
}
bool ButtonTest::run()
{
    close();
    const auto area = getVisualArea();
    if (lv_area_get_width(&area) != 400 || lv_area_get_height(&area) != 300) return false;
    const esp_err_t ret = bsp_button_test_init();
    if (ret != ESP_OK) {
        ESP_LOGE("button_test", "Initialize keys: %s", esp_err_to_name(ret));
        return false;
    }
    root_ = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(root_);
    lv_obj_set_pos(root_, area.x1, area.y1); lv_obj_set_size(root_, 400, 300);
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_color(root_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    rlcd_app_header_create(root_, "Button Test", &rlcd_header_button_test, backEvent, this);
    cards_[0] = createCard("Custom", "GPIO18 / Active low", 16);
    cards_[1] = createCard("BOOT", "GPIO0 / Active low", 208);
    summary_ = label(root_, "0 / 2 buttons passed", 16, 247, 310, &lv_font_montserrat_18);
    auto *reset_button = button(root_, LV_SYMBOL_REFRESH, 350, 238);
    lv_obj_add_event_cb(reset_button, resetEvent, LV_EVENT_CLICKED, this);
    progress_ = lv_bar_create(root_);
    lv_obj_remove_style_all(progress_);
    lv_obj_set_pos(progress_, 16, 281); lv_obj_set_size(progress_, 368, 5);
    lv_obj_set_style_border_width(progress_, 1, 0);
    lv_obj_set_style_border_color(progress_, lv_color_black(), 0);
    lv_obj_set_style_bg_color(progress_, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progress_, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_remove_flag(progress_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_bar_set_range(progress_, 0, 2);
    reset();
    timer_ = lv_timer_create(poll, SAMPLE_MS, this);
    if (!timer_) { close(); return false; }
    return true;
}
void ButtonTest::refresh()
{
    constexpr bsp_button_id_t ids[] = {BSP_BUTTON_ID_USER, BSP_BUTTON_ID_BOOT};
    int passed = 0;
    bool unavailable = false;
    for (unsigned i = 0; i < 2; ++i) {
        bool pressed = false;
        const auto ret = bsp_button_get_pressed(ids[i], &pressed);
        auto &card = cards_[i];
        auto &sequence = sequence_[i];
        if (sequence.update(ret == ESP_OK ? int(pressed) : -1)) {
            if (card.presses < MAX_COUNT) ++card.presses;
            ESP_LOGI("button_test", "%s passed, presses=%lu", i ? "BOOT" : "Custom",
                     static_cast<unsigned long>(card.presses));
        }
        passed += card.presses > 0;
        unavailable |= sequence.state < 0;
        if (card.shown_state == sequence.state && card.shown_count == card.presses) continue;
        card.shown_state = sequence.state; card.shown_count = card.presses;
        text(card.state, sequence.state < 0 ? "WAITING" : sequence.state ? "PRESSED" : "RELEASED");
        lv_obj_set_style_bg_color(card.state, sequence.state == 1 ? lv_color_black() : lv_color_white(), 0);
        lv_obj_set_style_text_color(card.state, sequence.state == 1 ? lv_color_white() : lv_color_black(), 0);
        char count[32];
        std::snprintf(count, sizeof(count), "Presses: %lu", static_cast<unsigned long>(card.presses));
        text(card.count, count);
        text(card.result, card.presses ? LV_SYMBOL_OK " PASS" : "NOT TESTED");
    }
    const int result = passed | (unavailable ? 4 : 0);
    if (shown_result_ == result) return;
    shown_result_ = result;
    char summary[32];
    std::snprintf(summary, sizeof(summary), "%d / 2 buttons passed", passed);
    text(summary_, summary);
    lv_bar_set_value(progress_, passed, LV_ANIM_OFF);
}
void ButtonTest::reset()
{
    sequence_[0] = sequence_[1] = Sequence{};
    for (auto &card : cards_) {
        card.presses = 0; card.shown_count = UINT32_MAX; card.shown_state = -2;
    }
    shown_result_ = -1;
    refresh();
}
void ButtonTest::backEvent(lv_event_t *event)
{ static_cast<ButtonTest *>(lv_event_get_user_data(event))->back(); }
void ButtonTest::resetEvent(lv_event_t *event)
{ static_cast<ButtonTest *>(lv_event_get_user_data(event))->reset(); }
void ButtonTest::poll(lv_timer_t *timer)
{ static_cast<ButtonTest *>(lv_timer_get_user_data(timer))->refresh(); }
bool ButtonTest::pause()
{
    if (timer_) lv_timer_pause(timer_);
    return true;
}
bool ButtonTest::resume()
{
    sequence_[0] = sequence_[1] = Sequence{};
    if (timer_) { refresh(); lv_timer_reset(timer_); lv_timer_resume(timer_); }
    return true;
}
bool ButtonTest::close()
{
    if (timer_) { lv_timer_delete(timer_); timer_ = nullptr; }
    if (root_) { lv_obj_delete(root_); root_ = nullptr; }
    summary_ = progress_ = nullptr;
    cards_[0] = cards_[1] = Card{};
    return true;
}
}
