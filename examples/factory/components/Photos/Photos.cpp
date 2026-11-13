/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * C5 Photos lifecycle, adapted to the RLCD's shared Brookesia app screen.
 */
#include "Photos.hpp"
#include "photo_init.h"
#include "photo_service.h"
#include <new>

LV_IMAGE_DECLARE(image_photos_mono);

namespace esp_brookesia::apps {
Photos *Photos::_instance = nullptr;
Photos *Photos::requestInstance(bool status_bar, bool navigation_bar)
{
    if (!_instance) _instance = new (std::nothrow) Photos(status_bar, navigation_bar);
    return _instance;
}
Photos::Photos(bool status_bar, bool navigation_bar)
    : App("Photos", &image_photos_mono, true, status_bar, navigation_bar) {}
Photos::~Photos() { _instance = nullptr; }
bool Photos::init() { return photo_service_start(); }
bool Photos::run()
{
    const lv_area_t area = getVisualArea();
    if (lv_area_get_width(&area) != 400 || lv_area_get_height(&area) != 300) return false;
    return init_photo_ui_screen(lv_screen_active(), area.x1, area.y1, on_back, this);
}
void Photos::on_back(void *context) { static_cast<Photos *>(context)->back(); }
bool Photos::back()
{
    if (photo_ui_back_to_list()) return true;
    return notifyCoreClosed();
}
bool Photos::close() { deinit_photo_ui(); return true; }
bool Photos::deinit() { deinit_photo_ui(); return true; }
bool Photos::pause() { pause_photo_ui(true); return true; }
bool Photos::resume() { pause_photo_ui(false); return true; }
}
