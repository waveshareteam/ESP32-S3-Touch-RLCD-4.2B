/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

// Use the same app lifecycle and installation interface as Games in the C5-2.8 project.
class Games : public systems::phone::App {
public:
    static Games *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~Games() override;
protected:
    Games(bool use_status_bar, bool use_navigation_bar);
    bool run() override;
    bool back() override;
    bool close() override;
    bool init() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;
private:
    static Games *_instance;
    static void on_back(void *context);
};

} // namespace esp_brookesia::apps
