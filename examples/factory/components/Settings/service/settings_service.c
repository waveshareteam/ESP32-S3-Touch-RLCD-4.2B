#include "settings_service.h"
#include "bsp/esp-bsp.h"
#include "esp_app_desc.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static TaskHandle_t worker_task;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static settings_snapshot_t snapshot;
static bool sd_requested, volume_save_requested;
static int volume_to_save;
static uint32_t volume_revision;

static void worker(void *unused)
{
    (void)unused;
    const esp_app_desc_t *app = esp_app_get_description();
    uint8_t mac[6] = {0};
    uint32_t flash_bytes = 0;
    char mac_text[18] = "--";
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        snprintf(mac_text, sizeof(mac_text), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    esp_flash_get_size(NULL, &flash_bytes);
    size_t psram_bytes = esp_psram_get_size();
    portENTER_CRITICAL(&lock);
    snprintf(snapshot.firmware, sizeof(snapshot.firmware), "%s", app->version);
    snprintf(snapshot.sdk, sizeof(snapshot.sdk), "%s", esp_get_idf_version());
    memcpy(snapshot.mac, mac_text, sizeof(mac_text));
    snapshot.flash_mb = flash_bytes / (1024 * 1024);
    snapshot.psram_mb = psram_bytes / (1024 * 1024);
    snapshot.about_ready = true;
    portEXIT_CRITICAL(&lock);
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        portENTER_CRITICAL(&lock);
        bool check_sd = sd_requested, save_volume = volume_save_requested;
        int volume = volume_to_save;
        uint32_t revision = volume_revision;
        sd_requested = volume_save_requested = false;
        portEXIT_CRITICAL(&lock);
        if (save_volume) {
            esp_err_t saved = bsp_audio_volume_save(volume);
            portENTER_CRITICAL(&lock);
            if (volume_revision == revision) snapshot.volume_save_failed = saved != ESP_OK;
            portEXIT_CRITICAL(&lock);
            if (saved != ESP_OK) ESP_LOGW("settings", "Save volume failed: %s", esp_err_to_name(saved));
        }
        if (!check_sd) continue;
        bsp_sdcard_info_t info = {0};
        esp_err_t ret = bsp_sdcard_mount();
        if (ret == ESP_OK) ret = bsp_sdcard_get_info(&info);
        portENTER_CRITICAL(&lock);
        snapshot.sd_state = ret == ESP_OK ? SETTINGS_SD_READY : SETTINGS_SD_ERROR;
        snapshot.sd_mounted = info.mounted;
        snapshot.sd_capacity = info.capacity_bytes;
        snapshot.sd_total = info.total_bytes;
        snapshot.sd_free = info.free_bytes;
        snprintf(snapshot.sd_error, sizeof(snapshot.sd_error), "%s", esp_err_to_name(ret));
        portEXIT_CRITICAL(&lock);
    }
}

bool settings_service_start(void)
{
    if (worker_task) return true;
    return xTaskCreate(worker, "rlcd_settings", 4096, NULL, 2, &worker_task) == pdPASS;
}

void settings_service_get_snapshot(settings_snapshot_t *out, uint16_t scan_first)
{
    if (!out) return;
    portENTER_CRITICAL(&lock);
    *out = snapshot;
    portEXIT_CRITICAL(&lock);
    out->volume = bsp_audio_volume_get();
    wifi_manager_get_snapshot(&out->wifi);
    wifi_manager_get_scan_snapshot(&out->scan, scan_first);
}

bool settings_service_set_volume(int volume, bool commit)
{
    if (!worker_task || bsp_audio_volume_set(volume) != ESP_OK) return false;
    if (commit) {
        portENTER_CRITICAL(&lock);
        volume_to_save = volume;
        ++volume_revision;
        volume_save_requested = true;
        snapshot.volume_save_failed = false;
        portEXIT_CRITICAL(&lock);
        xTaskNotifyGive(worker_task);
    }
    return true;
}

bool settings_service_scan(void) { return wifi_manager_request_scan(); }
bool settings_service_start_provisioning(void) { return wifi_manager_start_provisioning(); }
void settings_service_stop_provisioning(void) { wifi_manager_stop_provisioning(); }

bool settings_service_check_sd(void)
{
    if (!worker_task) return false;
    portENTER_CRITICAL(&lock);
    bool accepted = snapshot.sd_state != SETTINGS_SD_CHECKING;
    if (accepted) {
        snapshot.sd_state = SETTINGS_SD_CHECKING;
        sd_requested = true;
    }
    portEXIT_CRITICAL(&lock);
    if (accepted) xTaskNotifyGive(worker_task);
    return accepted;
}
