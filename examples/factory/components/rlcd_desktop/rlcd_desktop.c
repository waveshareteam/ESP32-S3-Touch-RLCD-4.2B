#include "rlcd_desktop.h"
#include "desktop_faces.h"
#include "bsp/esp-bsp.h"
#include "wifi_manager.h"
#include <stdio.h>
#include <string.h>

LV_IMAGE_DECLARE(esp_brookesia_image_small_status_bar_wifi_close_20_20);
LV_IMAGE_DECLARE(esp_brookesia_image_small_status_bar_wifi_level3_20_20);
LV_FONT_DECLARE(rlcd_font_status_16);

static struct {
    lv_obj_t *root, *clock, *face, *wifi, *battery_value, *battery_icon;
    lv_timer_t *timer;
    bool wifi_initialized, connected, clock_initialized;
    uint32_t clock_tick;
} ui;

static void text_if_changed(lv_obj_t *label, const char *value)
{
    if (label && strcmp(lv_label_get_text(label), value)) lv_label_set_text(label, value);
}

static lv_obj_t *label(int x, int y, int width, const char *text, const lv_font_t *font)
{
    lv_obj_t *obj = lv_label_create(ui.root);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, width);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_style_text_opa(obj, LV_OPA_COVER, 0);
    lv_label_set_text(obj, text);
    return obj;
}

static void poll(lv_timer_t *timer)
{
    (void)timer;
    if (!ui.root || lv_obj_get_screen(ui.root) != lv_screen_active()) return;
    if (!ui.clock_initialized || lv_tick_elaps(ui.clock_tick)>=1000) {
        struct tm local;
        if (wifi_manager_get_local_time(&local))
            rlcd_desktop_set_time(local.tm_hour,local.tm_min);
        else
            rlcd_desktop_set_time(-1,-1);
        bsp_battery_info_t battery;
        const bool battery_valid = bsp_battery_get_info(&battery) == ESP_OK;
        rlcd_desktop_set_battery(battery_valid ? battery.percent : -1, false);
        ui.clock_tick=lv_tick_get();
        ui.clock_initialized=true;
    }
    const lv_image_dsc_t *face = (bsp_user_button_get_press_count() & 1) ?
        &desktop_face_neutral : &desktop_face_smile;
    if (lv_image_get_src(ui.face) != face) lv_image_set_src(ui.face, face);
    const bool connected = wifi_manager_is_connected();
    if (!ui.wifi_initialized || connected != ui.connected) {
        lv_image_set_src(ui.wifi, connected ?
            &esp_brookesia_image_small_status_bar_wifi_level3_20_20 :
            &esp_brookesia_image_small_status_bar_wifi_close_20_20);
        ui.connected = connected;
        ui.wifi_initialized = true;
    }
}

static void deleted(lv_event_t *event)
{
    if (lv_event_get_target(event) != ui.root) return;
    if (ui.timer) lv_timer_delete(ui.timer);
    memset(&ui, 0, sizeof(ui));
}

void rlcd_desktop_destroy(void)
{
    if (ui.root) lv_obj_delete(ui.root);
}

bool rlcd_desktop_create(lv_obj_t *desktop)
{
    if (!desktop) return false;
    rlcd_desktop_destroy();
    ui.root = lv_obj_create(desktop);
    lv_obj_remove_style_all(ui.root);
    lv_obj_set_pos(ui.root, 0, 0);
    lv_obj_set_size(ui.root, 400, 32);
    lv_obj_set_style_bg_color(ui.root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui.root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(ui.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui.root, deleted, LV_EVENT_DELETE, NULL);
    ui.clock = label(16, 7, 74, "--:--", &rlcd_font_status_16);
    ui.face = lv_image_create(ui.root);
    lv_image_set_src(ui.face, &desktop_face_smile);
    lv_obj_set_pos(ui.face, 184, 0);
    lv_obj_remove_flag(ui.face, LV_OBJ_FLAG_CLICKABLE);
    ui.wifi = lv_image_create(ui.root);
    lv_obj_set_pos(ui.wifi, 286, 6);
    lv_obj_set_style_image_recolor(ui.wifi, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(ui.wifi, LV_OPA_COVER, 0);
    ui.battery_value = label(311, 7, 46, "--%", &rlcd_font_status_16);
    lv_obj_set_style_text_align(ui.battery_value, LV_TEXT_ALIGN_RIGHT, 0);
    ui.battery_icon = label(362, 7, 26, LV_SYMBOL_BATTERY_EMPTY, &lv_font_montserrat_16);
    ui.timer = lv_timer_create(poll, 50, NULL);
    if (!ui.timer) { rlcd_desktop_destroy(); return false; }
    poll(NULL);
    return true;
}

void rlcd_desktop_set_time(int hour, int minute)
{
    char text[8] = "--:--";
    if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60)
        snprintf(text, sizeof(text), "%02d:%02d", hour, minute);
    text_if_changed(ui.clock, text);
}

void rlcd_desktop_set_battery(int percent, bool charging)
{
    char text[8] = "--%";
    const char *icon = LV_SYMBOL_BATTERY_EMPTY;
    if (percent >= 0 && percent <= 100) {
        snprintf(text, sizeof(text), "%d%%", percent);
        icon = charging ? LV_SYMBOL_CHARGE : percent > 75 ? LV_SYMBOL_BATTERY_FULL :
            percent > 50 ? LV_SYMBOL_BATTERY_3 : percent > 25 ? LV_SYMBOL_BATTERY_2 :
            percent > 0 ? LV_SYMBOL_BATTERY_1 : LV_SYMBOL_BATTERY_EMPTY;
    }
    text_if_changed(ui.battery_value, text);
    text_if_changed(ui.battery_icon, icon);
}

