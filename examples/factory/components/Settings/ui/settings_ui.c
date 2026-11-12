/* C5 Settings behavior with native 400x300 black/white RLCD controls.
 * Radio and filesystem operations run outside the LVGL task. */
#include "settings_ui.h"
#include "settings_service.h"
#include "settings_icons.h"
#include "rlcd_app_header.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static struct {
    lv_obj_t *root, *body, *title;
    lv_obj_t *status, *action, *rows, *previous, *next;
    lv_obj_t *values[5], *qr;
    lv_obj_t *volume_slider, *volume_value, *volume_note;
    int volume_current;
    bool volume_dirty;
    lv_timer_t *timer;
    settings_close_cb_t close;
    void *context;
    settings_page_t page;
    unsigned scan_page;
    uint32_t scan_revision;
    bool portal_owned, portal_seen, scan_rejected;
    char qr_ssid[33];
} ui;

static void show_page(settings_page_t page);
static void poll(lv_timer_t *unused);

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, bool border)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    if (border) {
        lv_obj_set_style_border_width(obj, 1, 0);
        lv_obj_set_style_border_color(obj, lv_color_black(), 0);
        lv_obj_set_style_radius(obj, 8, 0);
    }
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, int w, const lv_font_t *font)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y); lv_obj_set_width(obj, w);
    lv_obj_set_height(obj, font->line_height);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_label_set_text(obj, text);
    return obj;
}

static void set_text(lv_obj_t *obj, const char *text)
{
    if (obj && strcmp(lv_label_get_text(obj), text)) lv_label_set_text(obj, text);
}

static lv_obj_t *image(lv_obj_t *parent, const lv_image_dsc_t *src, int x, int y)
{
    lv_obj_t *obj = lv_image_create(parent);
    lv_image_set_src(obj, src); lv_obj_set_pos(obj, x, y);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int w, int h,
                        lv_event_cb_t callback, void *data)
{
    lv_obj_t *obj = box(parent, x, y, w, h, true);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_width(obj, 2, LV_STATE_PRESSED);
    lv_obj_set_style_opa(obj, LV_OPA_COVER, LV_STATE_DISABLED);
    if (text) {
        lv_obj_t *caption = label(obj, text, 0, 0, w - 6, &lv_font_montserrat_16);
        lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(caption);
    }
    lv_obj_add_event_cb(obj, callback, LV_EVENT_CLICKED, data);
    return obj;
}

static void go_event(lv_event_t *event)
{
    show_page((settings_page_t)(intptr_t)lv_event_get_user_data(event));
}

static void back_event(lv_event_t *event)
{
    (void)event;
    if (ui.close) ui.close(ui.context);
}

static void tile(const char *title, const char *detail, const lv_image_dsc_t *icon,
                 int x, int y, settings_page_t page)
{
    lv_obj_t *obj = button(ui.body, NULL, x, y, 183, 75, go_event, (void *)(intptr_t)page);
    lv_obj_set_style_radius(obj, 6, 0);
    image(obj, icon, 8, 10);
    label(obj, title, 46, 13, 130, &lv_font_montserrat_14);
    label(obj, detail, 46, 39, 130, &lv_font_montserrat_12);
}

static void volume_caption(int volume)
{
    char text[12]; snprintf(text, sizeof(text), "%d%%", volume);
    set_text(ui.volume_value, text);
}

static void commit_volume(void)
{
    if (!ui.volume_dirty) return;
    if (settings_service_set_volume(ui.volume_current, true)) ui.volume_dirty = false;
    else set_text(ui.volume_note, "Save failed");
}

static void volume_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_VALUE_CHANGED) {
        ui.volume_current = (int)lv_slider_get_value(ui.volume_slider) * 5;
        ui.volume_dirty = true;
        settings_service_set_volume(ui.volume_current, false);
        volume_caption(ui.volume_current);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED) {
        commit_volume();
    }
}

static void create_volume(void)
{
    label(ui.body, LV_SYMBOL_VOLUME_MAX, 20, 10, 22, &lv_font_montserrat_18);
    label(ui.body, "Volume", 48, 12, 82, &lv_font_montserrat_14);
    ui.volume_note = label(ui.body, "", 144, 13, 142, &lv_font_montserrat_12);
    lv_obj_set_style_text_align(ui.volume_note, LV_TEXT_ALIGN_RIGHT, 0);
    ui.volume_value = label(ui.body, "", 290, 12, 90, &lv_font_montserrat_14);
    lv_obj_set_style_text_align(ui.volume_value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *track = box(ui.body, 20, 53, 360, 1, false);
    lv_obj_set_style_bg_color(track, lv_color_black(), 0);
    ui.volume_slider = lv_slider_create(ui.body);
    lv_obj_remove_style_all(ui.volume_slider);
    lv_obj_set_pos(ui.volume_slider, 20, 51); lv_obj_set_size(ui.volume_slider, 360, 5);
    lv_slider_set_range(ui.volume_slider, 0, 20);
    lv_obj_set_style_bg_color(ui.volume_slider, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ui.volume_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui.volume_slider, lv_color_black(), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(ui.volume_slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(ui.volume_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(ui.volume_slider, 6, LV_PART_KNOB);
    lv_obj_set_style_border_color(ui.volume_slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_border_width(ui.volume_slider, 2, LV_PART_KNOB);
    lv_obj_set_ext_click_area(ui.volume_slider, 20);
    lv_obj_remove_flag(ui.volume_slider, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui.volume_slider, volume_event, LV_EVENT_ALL, NULL);
}

static void update_volume(const settings_snapshot_t *snapshot)
{
    if (!ui.volume_dirty && !lv_slider_is_dragged(ui.volume_slider)) {
        ui.volume_current = snapshot->volume;
        int step = (snapshot->volume + 2) / 5;
        if (lv_slider_get_value(ui.volume_slider) != step) lv_slider_set_value(ui.volume_slider, step, LV_ANIM_OFF);
        volume_caption(snapshot->volume);
    }
    set_text(ui.volume_note, snapshot->volume_save_failed ? "Save failed" : "");
}

static void enable(lv_obj_t *obj, bool enabled)
{
    if (enabled) lv_obj_remove_state(obj, LV_STATE_DISABLED);
    else lv_obj_add_state(obj, LV_STATE_DISABLED);
}

static void scan_event(lv_event_t *event)
{
    (void)event;
    ui.scan_page = 0;
    ui.scan_rejected = !settings_service_scan();
    if (ui.scan_rejected) {
        settings_snapshot_t snapshot;
        settings_service_get_snapshot(&snapshot, 0);
        ui.scan_rejected = !snapshot.scan.running;
    }
    ui.scan_revision = UINT32_MAX;
    poll(NULL);
}

static void page_scan_event(lv_event_t *event)
{
    intptr_t direction = (intptr_t)lv_event_get_user_data(event);
    if (direction < 0 && ui.scan_page) --ui.scan_page;
    if (direction > 0) ++ui.scan_page;
    ui.scan_revision = UINT32_MAX;
    poll(NULL);
}

static void sd_event(lv_event_t *event)
{
    (void)event;
    if (!settings_service_check_sd()) set_text(ui.values[0], "Busy; try again");
    poll(NULL);
}

static lv_obj_t *fact(const char *name, int y, int value_x, const lv_font_t *font)
{
    label(ui.body, name, 12, y + 9, value_x - 24, &lv_font_montserrat_14);
    lv_obj_t *value = label(ui.body, "--", value_x, y + 9, 388 - value_x, font);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
    return value;
}

static void rule(int y)
{
    lv_obj_t *obj = box(ui.body, 12, y, 376, 1, false);
    lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
}

static void update_scan(const wifi_scan_snapshot_t *scan)
{
    if (ui.scan_revision == scan->revision) return;
    ui.scan_revision = scan->revision;
    lv_obj_clean(ui.rows);
    char text[64];
    if (scan->running) strcpy(text, "Scanning...");
    else if (ui.scan_rejected) strcpy(text, "Wi-Fi busy. Try again.");
    else if (scan->error) strcpy(text, "Scan failed. Try again.");
    else if (!scan->count) strcpy(text, "No networks found");
    else {
        unsigned first = scan->first;
        ui.scan_page = first / WIFI_SCAN_PAGE_SIZE;
        unsigned end = first + scan->page_count;
        snprintf(text, sizeof(text), "%u-%u / %u networks", first + 1, end, scan->count);
        for (unsigned i = first; i < end; ++i) {
            const wifi_scan_record_t *ap = &scan->records[i - first];
            int y = (int)(i - first) * 48;
            label(ui.rows, LV_SYMBOL_WIFI, 0, y + 11, 24, &lv_font_montserrat_16);
            label(ui.rows, ap->ssid, 32, y, 245, &lv_font_montserrat_14);
            char meta[32]; snprintf(meta, sizeof(meta), "CH %u   %s", ap->channel, ap->security);
            label(ui.rows, meta, 32, y + 24, 242, &lv_font_montserrat_12);
            char signal[24]; snprintf(signal, sizeof(signal), "%d dBm", ap->rssi);
            lv_obj_t *rssi = label(ui.rows, signal, 277, y + 12, 96, &lv_font_montserrat_14);
            lv_obj_set_style_text_align(rssi, LV_TEXT_ALIGN_RIGHT, 0);
            if (i + 1 < end) {
                lv_obj_t *line = box(ui.rows, 0, y + 46, 376, 1, false);
                lv_obj_set_style_bg_color(line, lv_color_black(), 0);
            }
        }
    }
    set_text(ui.status, text);
    enable(ui.action, !scan->running);
    set_text(lv_obj_get_child(ui.action, 0), scan->running ? "Scanning..." : "Scan again");
    enable(ui.previous, !scan->running && ui.scan_page > 0);
    enable(ui.next, !scan->running && (ui.scan_page + 1) * WIFI_SCAN_PAGE_SIZE < scan->count);
}

static void format_bytes(char *text, size_t size, uint64_t bytes)
{
    if (bytes >= (1ULL << 30))
        snprintf(text, size, "%llu.%llu GiB", (unsigned long long)(bytes >> 30),
                 (unsigned long long)((bytes % (1ULL << 30)) * 10 / (1ULL << 30)));
    else snprintf(text, size, "%llu MiB", (unsigned long long)(bytes >> 20));
}

static void update_sd(const settings_snapshot_t *s)
{
    const char *status = "Not checked";
    if (s->sd_state == SETTINGS_SD_CHECKING) status = "Checking...";
    else if (s->sd_state == SETTINGS_SD_READY) status = "Mounted";
    else if (s->sd_state == SETTINGS_SD_ERROR) status = s->sd_mounted ? "Read failed" : "Mount failed";
    set_text(ui.values[0], status);
    set_text(ui.values[1], s->sd_state == SETTINGS_SD_READY ? "FAT" : "--");
    char total[32] = "--", free_bytes[32] = "--";
    if (s->sd_state == SETTINGS_SD_READY) {
        format_bytes(total, sizeof(total), s->sd_total);
        format_bytes(free_bytes, sizeof(free_bytes), s->sd_free);
    }
    set_text(ui.values[2], total); set_text(ui.values[3], free_bytes);
    enable(ui.action, s->sd_state != SETTINGS_SD_CHECKING);
    set_text(lv_obj_get_child(ui.action, 0), s->sd_state == SETTINGS_SD_CHECKING ? "Checking..." : "Check card");
}

static void update_about(const settings_snapshot_t *s)
{
    if (!s->about_ready) return;
    set_text(ui.values[0], s->firmware); set_text(ui.values[1], s->sdk);
    set_text(ui.values[2], s->mac);
    char memory[40];
    snprintf(memory, sizeof(memory), "%lu MB / %lu MB", (unsigned long)s->flash_mb, (unsigned long)s->psram_mb);
    set_text(ui.values[3], memory); set_text(ui.values[4], "400 x 300 / 1-bit");
}

static void update_provision(const wifi_manager_snapshot_t *net)
{
    if (net->provisioning) ui.portal_seen = true;
    if (net->provisioning && net->ap_ssid[0]) {
        set_text(ui.values[0], net->ap_ssid);
        if (ui.qr && strcmp(ui.qr_ssid, net->ap_ssid)) {
            char payload[128];
            /* Same WIFI payload as C5: scan joins the device AP, then its
             * captive portal accepts the router's SSID/password. */
            snprintf(payload, sizeof(payload), "WIFI:T:WPA;P:%s;S:%s;H:false;;",
                     WIFI_PROVISION_PASSWORD, net->ap_ssid);
            if (lv_qrcode_update(ui.qr, payload, strlen(payload)) == LV_RESULT_OK) {
                snprintf(ui.qr_ssid, sizeof(ui.qr_ssid), "%s", net->ap_ssid);
                lv_obj_remove_flag(ui.qr, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    if (ui.portal_seen && net->provisioning_success)
        set_text(ui.status, net->credentials_saved ? "Connected and saved" : "Connected; save failed");
    else if (!ui.portal_owned) set_text(ui.status, "Wi-Fi not ready. Go back and retry.");
    else if (net->connecting && net->provisioning) set_text(ui.status, "Connecting to your Wi-Fi...");
    else if (net->provisioning) {
        set_text(ui.status, strstr(net->message, "failed") ? "Connection failed. Retry on your phone." : "Waiting for phone...");
    } else set_text(ui.status, strstr(net->message, "Could not") ? "Hotspot failed. Go back and retry." : "Starting hotspot...");
}

static void poll(lv_timer_t *unused)
{
    (void)unused;
    if (!ui.root) return;
    settings_snapshot_t snapshot;
    settings_service_get_snapshot(&snapshot, ui.scan_page * WIFI_SCAN_PAGE_SIZE);
    switch (ui.page) {
    case SETTINGS_HOME: update_volume(&snapshot); break;
    case SETTINGS_WIFI_SCAN: update_scan(&snapshot.scan); break;
    case SETTINGS_SD: update_sd(&snapshot); break;
    case SETTINGS_ABOUT: update_about(&snapshot); break;
    case SETTINGS_QR: update_provision(&snapshot.wifi); break;
    default: break;
    }
}

static void show_page(settings_page_t page)
{
    commit_volume();
    if (ui.portal_owned) {
        settings_service_stop_provisioning();
        ui.portal_owned = false;
    }
    lv_obj_clean(ui.body);
    ui.page = page; ui.status = ui.action = ui.rows = ui.previous = ui.next = ui.qr = NULL;
    ui.volume_slider = ui.volume_value = ui.volume_note = NULL;
    memset(ui.values, 0, sizeof(ui.values));
    ui.qr_ssid[0] = 0; ui.portal_seen = false;
    switch (page) {
    case SETTINGS_HOME:
        set_text(ui.title, "Settings");
        create_volume();
        tile("Wi-Fi", "Scan only", &settings_icon_wifi, 12, 80, SETTINGS_WIFI_SCAN);
        tile("Network setup", "Scan QR", &settings_icon_qr, 205, 80, SETTINGS_QR);
        tile("SD Card", "Status / Storage", &settings_icon_sd, 12, 165, SETTINGS_SD);
        tile("About", "Device info", &settings_icon_info, 205, 165, SETTINGS_ABOUT);
        break;
    case SETTINGS_WIFI_SCAN:
        set_text(ui.title, "Wi-Fi scan");
        ui.status = label(ui.body, "Scanning...", 12, 12, 274, &lv_font_montserrat_14);
        label(ui.body, "2.4 GHz", 317, 12, 71, &lv_font_montserrat_14);
        ui.rows = box(ui.body, 12, 41, 376, 144, false);
        ui.previous = button(ui.body, LV_SYMBOL_LEFT, 12, 196, 44, 44, page_scan_event, (void *)(intptr_t)-1);
        ui.next = button(ui.body, LV_SYMBOL_RIGHT, 64, 196, 44, 44, page_scan_event, (void *)(intptr_t)1);
        ui.action = button(ui.body, "Scan again", 116, 196, 272, 44, scan_event, NULL);
        ui.scan_page = 0; ui.scan_revision = UINT32_MAX;
        scan_event(NULL);
        break;
    case SETTINGS_QR:
        set_text(ui.title, "Network setup");
        label(ui.body, "Scan to join the device hotspot", 12, 12, 376, &lv_font_montserrat_14);
        ui.qr = lv_qrcode_create(ui.body);
        lv_qrcode_set_size(ui.qr, 152);
        lv_qrcode_set_dark_color(ui.qr, lv_color_black());
        lv_qrcode_set_light_color(ui.qr, lv_color_white());
        lv_qrcode_set_quiet_zone(ui.qr, true);
        lv_obj_set_pos(ui.qr, 12, 45);
        lv_obj_add_flag(ui.qr, LV_OBJ_FLAG_HIDDEN);
        label(ui.body, "Hotspot", 178, 45, 210, &lv_font_montserrat_12);
        ui.values[0] = label(ui.body, "--", 178, 66, 210, &lv_font_montserrat_14);
        label(ui.body, "Password: " WIFI_PROVISION_PASSWORD, 178, 103, 210, &lv_font_montserrat_12);
        label(ui.body, "Then open", 178, 138, 210, &lv_font_montserrat_12);
        label(ui.body, "192.168.4.1", 178, 158, 210, &lv_font_montserrat_16);
        ui.status = label(ui.body, "Starting hotspot...", 12, 216, 376, &lv_font_montserrat_14);
        ui.portal_owned = settings_service_start_provisioning();
        break;
    case SETTINGS_SD:
        set_text(ui.title, "SD Card");
        ui.values[0] = fact("Status", 12, 140, &lv_font_montserrat_14); rule(54);
        ui.values[1] = fact("File system", 55, 140, &lv_font_montserrat_14); rule(97);
        ui.values[2] = fact("Total", 98, 140, &lv_font_montserrat_14); rule(140);
        ui.values[3] = fact("Free", 141, 140, &lv_font_montserrat_14);
        ui.action = button(ui.body, "Check card", 12, 196, 376, 44, sd_event, NULL);
        break;
    case SETTINGS_ABOUT:
        set_text(ui.title, "About");
        label(ui.body, "ESP32-S3-Touch-RLCD-4.2", 12, 12, 376, &lv_font_montserrat_16);
        const char *fields[] = {"Firmware", "ESP-IDF", "MAC", "Flash / PSRAM", "Display"};
        for (unsigned i = 0; i < 5; ++i) {
            ui.values[i] = fact(fields[i], 43 + (int)i * 37, 156, &lv_font_montserrat_12);
            if (i < 4) rule(79 + (int)i * 37);
        }
        break;
    }
    poll(NULL);
}

static void deleted(lv_event_t *event)
{
    if (lv_event_get_target(event) != ui.root) return;
    commit_volume();
    if (ui.timer) lv_timer_delete(ui.timer);
    if (ui.portal_owned) settings_service_stop_provisioning();
    memset(&ui, 0, sizeof(ui));
}

bool settings_ui_init(lv_obj_t *parent, int32_t x, int32_t y, settings_close_cb_t close, void *context)
{
    if (!parent) return false;
    settings_ui_deinit();
    if (!settings_service_start()) return false;
    ui.close = close; ui.context = context;
    ui.root = box(parent, x, y, 400, 300, false);
    lv_obj_add_event_cb(ui.root, deleted, LV_EVENT_DELETE, NULL);
    rlcd_app_header_t header = rlcd_app_header_create(ui.root, "Settings", &rlcd_header_settings, back_event, NULL);
    ui.title = header.title;
    ui.body = box(ui.root, 0, 48, 400, 252, false);
    ui.timer = lv_timer_create(poll, 250, NULL);
    if (!ui.timer) { settings_ui_deinit(); return false; }
    show_page(SETTINGS_HOME);
    return true;
}

void settings_ui_deinit(void) { if (ui.root) lv_obj_delete(ui.root); }

bool settings_ui_back(void)
{
    if (!ui.root || ui.page == SETTINGS_HOME) return false;
    show_page(SETTINGS_HOME);
    return true;
}

void settings_ui_pause(bool paused)
{
    if (!ui.timer) return;
    if (paused) {
        commit_volume();
        if (ui.portal_owned) show_page(SETTINGS_HOME);
        lv_timer_pause(ui.timer);
    } else {
        poll(NULL);
        lv_timer_resume(ui.timer);
    }
}
