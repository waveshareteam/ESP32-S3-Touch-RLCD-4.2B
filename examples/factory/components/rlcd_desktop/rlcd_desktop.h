#pragma once
#include <stdbool.h>
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Call with the LVGL lock held. Attach only to Brookesia's desktop container.
 * Time follows the network-synchronized system clock; battery reads BSP cache.
 * Charging state is indicated by the physical LED, not detected by this UI.
 * This component never initializes/reads RTC or starts time synchronization. */
bool rlcd_desktop_create(lv_obj_t *desktop);
void rlcd_desktop_destroy(void);
void rlcd_desktop_set_time(int hour, int minute);
void rlcd_desktop_set_battery(int percent, bool charging);
#ifdef __cplusplus
}
#endif

