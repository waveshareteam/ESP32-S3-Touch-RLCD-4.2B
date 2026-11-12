#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {
class Weather : public systems::phone::App {
public:
    static Weather *requestInstance(bool use_status_bar=false,bool use_navigation_bar=false);
    ~Weather() override;
protected:
    Weather(bool use_status_bar,bool use_navigation_bar);
    bool run() override;
    bool back() override;
    bool close() override;
    bool init() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;
private:
    static Weather *_instance;
    static void on_back(void *context);
};
}
