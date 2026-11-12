#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "wifi_manager.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { SETTINGS_SD_UNCHECKED, SETTINGS_SD_CHECKING, SETTINGS_SD_READY, SETTINGS_SD_ERROR } settings_sd_state_t;
typedef struct {
    bool about_ready;
    int volume;
    bool volume_save_failed;
    char firmware[33], sdk[33], mac[18];
    uint32_t flash_mb, psram_mb;
    settings_sd_state_t sd_state;
    bool sd_mounted;
    uint64_t sd_capacity, sd_total, sd_free;
    char sd_error[40];
    wifi_manager_snapshot_t wifi;
    wifi_scan_snapshot_t scan;
} settings_snapshot_t;

bool settings_service_start(void);
void settings_service_get_snapshot(settings_snapshot_t *out, uint16_t scan_first);
/* Set the live volume without hardware/flash I/O on the UI task.
 * commit queues persistence of this exact released value. */
bool settings_service_set_volume(int volume, bool commit);
bool settings_service_scan(void);
bool settings_service_start_provisioning(void);
void settings_service_stop_provisioning(void);
bool settings_service_check_sd(void);
#ifdef __cplusplus
}
#endif
