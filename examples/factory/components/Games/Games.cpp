/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adapted from C5-T-2.8-Factory/components/Games for the 400x300 RLCD.
 */
#include "Games.hpp"
#include "game_init.h"
#include <new>

LV_IMAGE_DECLARE(image_2048_mono);

namespace esp_brookesia::apps {

Games *Games::_instance = nullptr;

Games *Games::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new (std::nothrow) Games(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

Games::Games(bool use_status_bar, bool use_navigation_bar)
    : App("2048", &image_2048_mono, true, use_status_bar, use_navigation_bar)
{
}

Games::~Games() { _instance = nullptr; }

bool Games::run()
{
    const lv_area_t area = getVisualArea();
    if (lv_area_get_width(&area) != 400 || lv_area_get_height(&area) != 300) {
        return false;
    }
    // Use the default app screen managed by Brookesia without creating or switching to a separate screen.
    return init_game_ui_screen(lv_screen_active(), area.x1, area.y1, on_back, this);
}

void Games::on_back(void *context) { static_cast<Games *>(context)->back(); }
bool Games::back() { return notifyCoreClosed(); }
bool Games::close() { deinit_game_ui_screen(); return true; }
bool Games::init() { return true; }
bool Games::deinit() { deinit_game_ui_screen(); return true; }
bool Games::pause() { pause_game_ui_screen(true); return true; }
bool Games::resume() { pause_game_ui_screen(false); return true; }

} // namespace esp_brookesia::apps
