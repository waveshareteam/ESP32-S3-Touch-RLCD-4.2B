#include "Weather.hpp"
#include "weather_init.h"
#include "weather_service.h"
#include "weather_icons.h"
#include <new>

namespace esp_brookesia::apps {
Weather *Weather::_instance=nullptr;
Weather *Weather::requestInstance(bool status_bar,bool navigation_bar)
{
    if(!_instance) _instance=new(std::nothrow) Weather(status_bar,navigation_bar);
    return _instance;
}
Weather::Weather(bool status_bar,bool navigation_bar)
    : App("Weather",&image_weather_mono,true,status_bar,navigation_bar) {}
Weather::~Weather() { _instance=nullptr; }
bool Weather::run()
{
    const lv_area_t area=getVisualArea();
    if(lv_area_get_width(&area)!=400 || lv_area_get_height(&area)!=300) return false;
    return init_weather_ui_screen(lv_screen_active(),area.x1,area.y1,on_back,this);
}
void Weather::on_back(void *context) { static_cast<Weather *>(context)->back(); }
bool Weather::back() { return notifyCoreClosed(); }
bool Weather::close() { deinit_weather_ui(); return true; }
bool Weather::init() { return weather_service_start(); }
bool Weather::deinit() { deinit_weather_ui(); return true; }
bool Weather::pause() { pause_weather_ui(true); return true; }
bool Weather::resume() { pause_weather_ui(false); return true; }
}
