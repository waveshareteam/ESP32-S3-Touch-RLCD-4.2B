#pragma once

#include "lvgl.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

// Use the same app class interface as 08_bookesia; this class can serve as a porting reference for other apps.
class DrawingBoard : public systems::phone::App {
public:
    static DrawingBoard *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~DrawingBoard() override;

protected:
    DrawingBoard(bool use_status_bar, bool use_navigation_bar);
    bool run() override;
    bool back() override;
    bool close() override;
    bool init() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;

private:
    static DrawingBoard *_instance;
    static void back_event_cb(lv_event_t *event);
};

} // namespace esp_brookesia::apps
