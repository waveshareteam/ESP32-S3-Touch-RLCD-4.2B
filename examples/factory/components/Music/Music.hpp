#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"
namespace esp_brookesia::apps {
class Music : public systems::phone::App {
public:
    static Music *requestInstance(bool status_bar=false,bool navigation_bar=false);
    ~Music() override;
protected:
    Music(bool status_bar,bool navigation_bar);
    bool init() override;
    bool run() override;
    bool back() override;
    bool close() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;
private:
    static Music *_instance;
    static void on_back(void *ctx);
};
}
