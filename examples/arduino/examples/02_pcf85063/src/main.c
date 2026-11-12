/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include "example_support.h"
#include "clock_font.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "rtc_clock";
static const bsp_sensor_datetime_t initial_time = {2026, 11, 12, 4, 16, 0, 0};
static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static struct {
    bool ready, pending, online, synced, rtc_valid;
    unsigned generation, sntp_generation;
    bsp_sensor_datetime_t rtc;
} state;
static lv_obj_t *time_label, *date_label;

static void synchronized(struct timeval *tv)
{
    (void)tv;
    portENTER_CRITICAL(&state_lock);
    bool accepted = state.online && state.generation == state.sntp_generation;
    bool first_sync = accepted && !state.synced;
    if (accepted) state.synced = true;
    portEXIT_CRITICAL(&state_lock);
    if (first_sync) ESP_LOGI(TAG, "SNTP synchronized; displaying Beijing time (UTC+8)");
    // The hardware RTC deliberately keeps its independent preset timeline.
}

static void network_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    bool lost_connection = false, connected = false;
    portENTER_CRITICAL(&state_lock);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) state.ready = true;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        lost_connection = state.online;
        state.pending = state.online = state.synced = false;
        ++state.generation;
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        connected = !state.online;
        state.pending = false;
        state.online = true;
        state.synced = false;
        ++state.generation;
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_LOST_IP) {
        lost_connection = state.online;
        state.online = state.synced = false;
        // The AP may still be associated. Wait for a new IP or a disconnect event.
        state.pending = true;
        ++state.generation;
    }
    portEXIT_CRITICAL(&state_lock);
    // Failed reconnect attempts are expected while offline; report only transitions.
    if (lost_connection) ESP_LOGW(TAG, "Network unavailable; displaying the continuously running RTC");
    if (connected) ESP_LOGI(TAG, "Wi-Fi connected; waiting for SNTP synchronization");
}

static void start_network(void)
{
    example_nvs_init();
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t event_loop = esp_event_loop_create_default();
    ESP_ERROR_CHECK(event_loop == ESP_ERR_INVALID_STATE ? ESP_OK : event_loop);
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_sta() ? ESP_OK : ESP_ERR_NO_MEM);
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, network_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, network_event, NULL));
    wifi_config_t config = {.sta = {.ssid = "weather-wifi", .password = "12345678"}};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static esp_err_t start_sntp(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,
        ESP_SNTP_SERVER_LIST("ntp.aliyun.com", "pool.ntp.org"));
    config.wait_for_sync = false;
    config.sync_cb = synchronized;
    return esp_netif_sntp_init(&config);
}

static void refresh(lv_timer_t *timer)
{
    (void)timer;
    bsp_sensor_datetime_t rtc;
    bool network, valid;
    portENTER_CRITICAL(&state_lock);
    network = state.online && state.synced;
    valid = state.rtc_valid;
    rtc = state.rtc;
    portEXIT_CRITICAL(&state_lock);
    char clock_text[24] = "--:--", date_text[32] = "----.--.--";
    int second = 0;
    if (network) {
        time_t now = time(NULL);
        struct tm local;
        localtime_r(&now, &local);
        strftime(clock_text, sizeof(clock_text), "%H:%M", &local);
        strftime(date_text, sizeof(date_text), "%Y.%m.%d", &local);
        second = local.tm_sec;
    } else if (valid) {
        snprintf(clock_text, sizeof(clock_text), "%02d:%02d", rtc.hour, rtc.minute);
        snprintf(date_text, sizeof(date_text), "%04d.%02d.%02d", rtc.year, rtc.month, rtc.day);
        second = rtc.second;
    }
    // Space and colon have the same advance, so blinking never shifts the digits.
    if ((network || valid) && (second & 1)) clock_text[2] = ' ';
    example_label_update(time_label, clock_text);
    example_label_update(date_label, date_text);
}

void example_main(void)
{
    setenv("TZ", "CST-8", 1);
    tzset();
    lv_obj_t *screen = example_screen_begin(NULL);
    time_label = example_label(screen, "--:--", 0, 90, 400, &clock_font_180);
    lv_obj_set_height(time_label, 180);
    lv_label_set_long_mode(time_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_letter_space(time_label, 0, 0);
    date_label = example_label(screen, "----.--.--", 20, 26, 360, &lv_font_montserrat_26);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_timer_create(refresh, 200, NULL);
    bsp_display_unlock();
    start_network();
    bool rtc_initialized = false, sntp_running = false;
    unsigned active_generation = 0, second = 0;
    esp_err_t last_connect_error = ESP_OK;
    for (;;) {
        // Hardware and network work run on the internal-RAM main stack, outside the GUI lock.
        if (!rtc_initialized && second % 5 == 0) {
            esp_err_t err = pcf_init();
            if (err == ESP_OK) err = bsp_sensor_set_datetime(&initial_time);
            rtc_initialized = err == ESP_OK;
            if (err != ESP_OK) ESP_LOGW(TAG, "RTC initialization: %s", esp_err_to_name(err));
            else ESP_LOGI(TAG, "RTC preset: 2026-11-12 16:00:00; no network writeback");
        }
        bsp_sensor_datetime_t rtc = {0};
        esp_err_t rtc_error = rtc_initialized ? bsp_sensor_read_datetime(&rtc) : ESP_ERR_INVALID_STATE;
        portENTER_CRITICAL(&state_lock);
        state.rtc_valid = rtc_error == ESP_OK;
        if (state.rtc_valid) state.rtc = rtc;
        bool online = state.online;
        unsigned generation = state.generation;
        bool connect = state.ready && !state.online && !state.pending && second % 5 == 0;
        if (connect) state.pending = true;
        portEXIT_CRITICAL(&state_lock);
        if (rtc_initialized && rtc_error != ESP_OK && second % 10 == 0)
            ESP_LOGW(TAG, "RTC read: %s", esp_err_to_name(rtc_error));
        if (sntp_running && (!online || active_generation != generation)) {
            esp_netif_sntp_deinit();
            sntp_running = false;
        }
        if (online && !sntp_running) {
            portENTER_CRITICAL(&state_lock);
            state.sntp_generation = generation;
            portEXIT_CRITICAL(&state_lock);
            esp_err_t err = start_sntp();
            sntp_running = err == ESP_OK;
            active_generation = generation;
            if (err != ESP_OK) ESP_LOGW(TAG, "SNTP start: %s", esp_err_to_name(err));
        }
        if (connect) {
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) {
                portENTER_CRITICAL(&state_lock);
                state.pending = false;
                portEXIT_CRITICAL(&state_lock);
                if (err != last_connect_error) ESP_LOGW(TAG, "Wi-Fi connect: %s", esp_err_to_name(err));
            }
            last_connect_error = err;
        }
        ++second;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
