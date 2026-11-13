#include "Settings.hpp"
#include "settings_ui.h"
#include "settings_icons.h"
#include <new>

namespace esp_brookesia::apps {
Settings *Settings::_instance = nullptr;
Settings *Settings::requestInstance(bool status_bar, bool navigation_bar)
{
    if (!_instance) _instance = new (std::nothrow) Settings(status_bar, navigation_bar);
    return _instance;
}
Settings::Settings(bool status_bar, bool navigation_bar)
    : App("Settings", &settings_icon_desktop, true, status_bar, navigation_bar) {}
Settings::~Settings() { _instance = nullptr; }
bool Settings::init() { return true; }
bool Settings::run()
{
    const lv_area_t area = getVisualArea();
    if (lv_area_get_width(&area) != 400 || lv_area_get_height(&area) != 300) return false;
    return settings_ui_init(lv_screen_active(), area.x1, area.y1, on_back, this);
}
void Settings::on_back(void *context) { static_cast<Settings *>(context)->back(); }
bool Settings::back()
{
    if (settings_ui_back()) return true;
    return notifyCoreClosed();
}
bool Settings::close() { settings_ui_deinit(); return true; }
bool Settings::deinit() { settings_ui_deinit(); return true; }
bool Settings::pause() { settings_ui_pause(true); return true; }
bool Settings::resume() { settings_ui_pause(false); return true; }
}
