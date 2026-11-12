/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * C5 Sensor app lifecycle adapted to the shared 400x300 RLCD screen.
 */
#include "Sensor.hpp"
#include "sensor_init.h"
#include <new>

LV_IMAGE_DECLARE(image_sensor_mono);
namespace esp_brookesia::apps {
Sensor *Sensor::_instance = nullptr;
Sensor *Sensor::requestInstance(bool status_bar, bool navigation_bar)
{
    if (!_instance) _instance = new (std::nothrow) Sensor(status_bar, navigation_bar);
    return _instance;
}
Sensor::Sensor(bool status_bar, bool navigation_bar)
    : App("Sensor", &image_sensor_mono, true, status_bar, navigation_bar) {}
Sensor::~Sensor() { _instance = nullptr; }
bool Sensor::init() { return true; }
bool Sensor::run()
{
    const lv_area_t area = getVisualArea();
    if (lv_area_get_width(&area) != 400 || lv_area_get_height(&area) != 300) return false;
    return init_sensor_ui_screen(lv_screen_active(), area.x1, area.y1, on_back, this);
}
void Sensor::on_back(void *context) { static_cast<Sensor *>(context)->back(); }
bool Sensor::back() { return notifyCoreClosed(); }
bool Sensor::close() { deinit_sensor_ui(); return true; }
bool Sensor::deinit() { deinit_sensor_ui(); return true; }
bool Sensor::pause() { pause_sensor_ui(true); return true; }
bool Sensor::resume() { pause_sensor_ui(false); return true; }
}
