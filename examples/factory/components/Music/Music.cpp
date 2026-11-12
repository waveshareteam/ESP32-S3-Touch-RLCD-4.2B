#include "Music.hpp"
#include "music_ui.h"
#include "music_assets.h"
#include <new>
namespace esp_brookesia::apps {
Music *Music::_instance=nullptr;
Music *Music::requestInstance(bool status_bar,bool navigation_bar)
{
    if(!_instance) _instance=new(std::nothrow) Music(status_bar,navigation_bar);
    return _instance;
}
Music::Music(bool status_bar,bool navigation_bar)
    : App("Music",&music_icon_desktop,true,status_bar,navigation_bar) {}
Music::~Music() { _instance=nullptr; }
bool Music::init() { return true; }
bool Music::run()
{
    const lv_area_t area=getVisualArea();
    if(lv_area_get_width(&area)!=400 || lv_area_get_height(&area)!=300) return false;
    return music_ui_init(lv_screen_active(),area.x1,area.y1,on_back,this);
}
void Music::on_back(void *ctx) { static_cast<Music *>(ctx)->back(); }
bool Music::back() { return notifyCoreClosed(); }
bool Music::close() { music_ui_deinit(); return true; }
bool Music::deinit() { music_ui_deinit(); return true; }
bool Music::pause() { music_ui_pause(true); return true; }
bool Music::resume() { music_ui_pause(false); return true; }
}
