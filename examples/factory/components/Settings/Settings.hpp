/* C5 Settings APP lifecycle adapted to the 400x300 RLCD. */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {
class Settings : public systems::phone::App {
public:
    static Settings *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
    ~Settings() override;
protected:
    Settings(bool use_status_bar, bool use_navigation_bar);
    bool init() override;
    bool run() override;
    bool back() override;
    bool close() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;
private:
    static Settings *_instance;
    static void on_back(void *context);
};
}
