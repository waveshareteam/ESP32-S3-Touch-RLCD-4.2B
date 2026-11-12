/* SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {
class DisplayTest final : public systems::phone::App {
public:
    static DisplayTest *requestInstance();
    ~DisplayTest() override;
protected:
    bool init() override { return true; }
    bool run() override;
    bool back() override { return notifyCoreClosed(); }
    bool close() override;
    bool deinit() override { return close(); }
    bool pause() override;
    bool resume() override;
private:
    DisplayTest();
    static DisplayTest *instance_;
    static void cycle(lv_timer_t *timer);
    static void touchEvent(lv_event_t *event);
    lv_obj_t *root_ = nullptr;
    lv_timer_t *timer_ = nullptr;
    bool white_ = false, tap_pending_ = false;
    uint32_t tap_tick_ = 0;
};
}
