/* SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {
class ButtonTest final : public systems::phone::App {
public:
    static ButtonTest *requestInstance();
    ~ButtonTest() override;
protected:
    bool init() override { return true; }
    bool run() override;
    bool back() override { return notifyCoreClosed(); }
    bool close() override;
    bool deinit() override { return close(); }
    bool pause() override;
    bool resume() override;
private:
    struct Sequence {
        enum Phase { InitialRelease, Press, FinalRelease } phase = InitialRelease;
        int candidate = -1, state = -1;
        unsigned samples = 0;
        bool update(int raw);
    };
    struct Card {
        lv_obj_t *state = nullptr, *count = nullptr, *result = nullptr;
        int shown_state = -2;
        uint32_t presses = 0, shown_count = UINT32_MAX;
    };
    ButtonTest();
    static ButtonTest *instance_;
    static void backEvent(lv_event_t *event);
    static void resetEvent(lv_event_t *event);
    static void poll(lv_timer_t *timer);
    Card createCard(const char *name, const char *signal, int x);
    void reset();
    void refresh();
    lv_obj_t *root_ = nullptr, *summary_ = nullptr, *progress_ = nullptr;
    lv_timer_t *timer_ = nullptr;
    Sequence sequence_[2];
    Card cards_[2];
    int shown_result_ = -1;
};
}
