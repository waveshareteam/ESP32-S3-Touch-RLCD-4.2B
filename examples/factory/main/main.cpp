/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <new>
#include <vector>

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_brookesia.hpp"
#include "DrawingBoard.hpp"
#include "Games.hpp"
#include "Weather.hpp"
#include "Photos.hpp"
#include "Sensor.hpp"
#include "Settings.hpp"
#include "settings_service.h"
#include "Music.hpp"
#include "ButtonTest.hpp"
#include "DisplayTest.hpp"
#include "MicTest.hpp"
#include "XiaozhiApp.hpp"
#include "wifi_manager.h"
#include "stylesheet.hpp"
#include "rlcd_desktop.h"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;
using namespace esp_brookesia::apps;

static const char *TAG = "main";

// false: show the app desktop; true: open the drawing board directly at startup.
constexpr bool EXAMPLE_AUTO_START_DRAWING = false;

extern "C" void app_main(void)
{
    // 1. Initialize core services and make NVS available to apps.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = bsp_audio_volume_init();
    if (ret != ESP_OK) ESP_LOGW(TAG, "Load volume failed; using default: %s", esp_err_to_name(ret));

    ret = bsp_user_button_init();
    if (ret != ESP_OK) ESP_LOGW(TAG, "GPIO18 button init failed: %s", esp_err_to_name(ret));

    ret = bsp_battery_init();
    if (ret != ESP_OK) ESP_LOGW(TAG, "Battery ADC init failed: %s", esp_err_to_name(ret));

    // Prefer saved Wi-Fi credentials; use the test access point if unconfigured and retry in the background after disconnection.
    if (!wifi_manager_init()) {
        ESP_LOGE(TAG, "Start network service failed");
    }

    // 2. Initialize the display, LVGL, and touch using the existing S3 BSP.
    lv_display_t *display = bsp_display_start();
    if (display == nullptr) {
        ESP_LOGE(TAG, "Start display/LVGL/touch failed");
        return;
    }

    // Detect SD storage in the background so Settings can show the boot result.
    if (!settings_service_start()) {
        ESP_LOGW(TAG, "Start settings service failed; retry when Settings opens");
    }

    // 3. Connect the Brookesia GUI lock to the BSP; this BSP returns bool and uses a zero timeout to wait indefinitely.
    LvLock::registerCallbacks([](int timeout_ms) {
        return bsp_display_lock(timeout_ms > 0 ? static_cast<uint32_t>(timeout_ms) : 0);
    }, []() {
        bsp_display_unlock();
        return true;
    });
    LvLockGuard gui_guard;

    // 4. Create the app framework and load the 400x300 monochrome theme.
    Phone *phone = new (std::nothrow) Phone(display);
    if (phone == nullptr) {
        ESP_LOGE(TAG, "Create Phone failed");
        return;
    }
    static const Stylesheet stylesheet = make_rlcd_stylesheet();
    if (!phone->addStylesheet(stylesheet) || !phone->activateStylesheet(stylesheet) || !phone->begin()) {
        ESP_LOGE(TAG, "Start Brookesia failed");
        delete phone;
        return;
    }

    // 5. Keep the reference project's registry interface for automatically registered apps.
    std::vector<systems::base::Manager::RegistryAppInfo> inited_apps;
    if (!phone->initAppFromRegistry(inited_apps) || !phone->installAppFromRegistry(inited_apps)) {
        ESP_LOGE(TAG, "Install registered apps failed");
        delete phone;
        return;
    }

    // 6. Existing first page, followed by Button Test and Display Test on page two.
    // installApp returns an app ID; only negative values indicate failure, so do not treat the result as a bool.
    auto app_DrawingBoard = DrawingBoard::requestInstance();
    if (app_DrawingBoard == nullptr) {
        ESP_LOGE(TAG, "Create DrawingBoard failed");
        delete phone;
        return;
    }
    const int drawing_app_id = phone->installApp(app_DrawingBoard);
    if (drawing_app_id < 0) {
        ESP_LOGE(TAG, "Install DrawingBoard failed");
        delete phone;
        return;
    }

    auto app_Xiaozhi = XiaozhiApp::requestInstance();
    int xiaozhi_app_id = -1;
    if (app_Xiaozhi == nullptr) {
        ESP_LOGE(TAG, "Create Xiaozhi failed; keeping the desktop and other apps");
    } else {
        xiaozhi_app_id = phone->installApp(app_Xiaozhi);
        if (xiaozhi_app_id < 0) {
            ESP_LOGE(TAG, "Install Xiaozhi failed; keeping the desktop and other apps");
            delete app_Xiaozhi;
        }
    }

    auto app_Weather = Weather::requestInstance();
    if (app_Weather == nullptr) {
        ESP_LOGE(TAG, "Create Weather failed");
        delete phone;
        return;
    }
    const int weather_app_id = phone->installApp(app_Weather);
    if (weather_app_id < 0) {
        ESP_LOGE(TAG, "Install Weather failed");
        delete phone;
        return;
    }

    auto app_Music = Music::requestInstance();
    if (app_Music == nullptr) {
        ESP_LOGE(TAG, "Create Music failed"); delete phone; return;
    }
    const int music_app_id = phone->installApp(app_Music);
    if (music_app_id < 0) {
        ESP_LOGE(TAG, "Install Music failed"); delete phone; return;
    }

    auto app_Games = Games::requestInstance();
    if (app_Games == nullptr) {
        ESP_LOGE(TAG, "Create Games failed");
        delete phone;
        return;
    }
    const int games_app_id = phone->installApp(app_Games);
    if (games_app_id < 0) {
        ESP_LOGE(TAG, "Install Games failed");
        delete phone;
        return;
    }

    auto app_Photos = Photos::requestInstance();
    if (app_Photos == nullptr) {
        ESP_LOGE(TAG, "Create Photos failed");
        delete phone;
        return;
    }
    const int photos_app_id = phone->installApp(app_Photos);
    if (photos_app_id < 0) {
        ESP_LOGE(TAG, "Install Photos failed");
        delete phone;
        return;
    }

    auto app_Sensor = Sensor::requestInstance();
    if (app_Sensor == nullptr) {
        ESP_LOGE(TAG, "Create Sensor failed");
        delete phone;
        return;
    }
    const int sensor_app_id = phone->installApp(app_Sensor);
    if (sensor_app_id < 0) {
        ESP_LOGE(TAG, "Install Sensor failed");
        delete phone;
        return;
    }

    auto app_Settings = Settings::requestInstance();
    if (app_Settings == nullptr) {
        ESP_LOGE(TAG, "Create Settings failed");
        delete phone;
        return;
    }
    const int settings_app_id = phone->installApp(app_Settings);
    if (settings_app_id < 0) {
        ESP_LOGE(TAG, "Install Settings failed");
        delete phone;
        return;
    }

    auto app_ButtonTest = ButtonTest::requestInstance();
    if (!app_ButtonTest) {
        ESP_LOGE(TAG, "Create Button Test failed"); delete phone; return;
    }
    const int button_test_app_id = phone->installApp(app_ButtonTest);
    if (button_test_app_id < 0) {
        ESP_LOGE(TAG, "Install Button Test failed"); delete phone; return;
    }
    auto app_DisplayTest = DisplayTest::requestInstance();
    if (!app_DisplayTest) {
        ESP_LOGE(TAG, "Create Display Test failed"); delete phone; return;
    }
    const int display_test_app_id = phone->installApp(app_DisplayTest);
    if (display_test_app_id < 0) {
        ESP_LOGE(TAG, "Install Display Test failed"); delete phone; return;
    }

    auto app_MicTest = MicTest::requestInstance();
    if (!app_MicTest) {
        ESP_LOGE(TAG, "Create Mic Test failed"); delete phone; return;
    }
    const int mic_test_app_id = phone->installApp(app_MicTest);
    if (mic_test_app_id < 0) {
        ESP_LOGE(TAG, "Install Mic Test failed"); delete phone; return;
    }

    // Desktop-only time/Wi-Fi/face header; APP screens keep their full visual area.
    if (!rlcd_desktop_create(phone->getDisplay().getMainScreenObject())) {
        ESP_LOGW(TAG, "Create desktop status bar failed; APPs remain available");
    }

    // To add another app, include its header and add its component dependency in main/CMakeLists.txt:
    // auto app_MyApp = MyApp::requestInstance();
    // const int my_app_id = phone->installApp(app_MyApp);

    // 7. Keep the direct app launch option; by default, tap Drawing on the desktop to open it.
    if constexpr (EXAMPLE_AUTO_START_DRAWING) {
        const systems::base::Context::AppEventData event = {
            .id = drawing_app_id,
            .type = systems::base::Context::AppEventType::START,
            .data = nullptr,
        };
        if (!phone->sendAppEvent(&event)) {
            ESP_LOGE(TAG, "Start DrawingBoard failed");
        }
    }

    ESP_LOGI(TAG, "Brookesia ready: 400x300, Drawing ID=%d, AIChats ID=%d, Weather ID=%d, Music ID=%d, 2048 ID=%d, Photos ID=%d, Sensor ID=%d, Settings ID=%d",
             drawing_app_id, xiaozhi_app_id, weather_app_id, music_app_id, games_app_id, photos_app_id, sensor_app_id, settings_app_id);
    ESP_LOGI(TAG, "Test apps ready: Button Test ID=%d, Display Test ID=%d, Mic Test ID=%d",
             button_test_app_id, display_test_app_id, mic_test_app_id);
    // gui_guard releases the display lock; LVGL and Brookesia continue managing the desktop and apps.
}
