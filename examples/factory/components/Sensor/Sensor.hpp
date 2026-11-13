/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {
class Sensor : public systems::phone::App {
public:
    static Sensor *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~Sensor() override;
protected:
    Sensor(bool use_status_bar, bool use_navigation_bar);
    bool init() override;
    bool run() override;
    bool back() override;
    bool close() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;
private:
    static Sensor *_instance;
    static void on_back(void *context);
};
}
