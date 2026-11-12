#include "sensor_init.h"
#include "sensor_service.h"
#include "rlcd_app_header.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

/* Format sensor snapshots for display; hardware access stays in the service. */
typedef struct {
    char temperature[16], humidity[16], date[20], time[16];
} sensor_text_t;

static bool sensor_date_valid(int year, int month, int day, int hour, int minute, int second)
{
    static const unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (year < 1970 || year > 2069 || month < 1 || month > 12 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;
    unsigned limit = days[month - 1];
    if (month == 2 && year % 4 == 0) ++limit;
    return day >= 1 && (unsigned)day <= limit;
}

static void sensor_format(const sensor_snapshot_t *s, sensor_text_t *out)
{
    memset(out, 0, sizeof(*out));
    strcpy(out->temperature, "--"); strcpy(out->humidity, "--");
    strcpy(out->date, "----.--.--"); strcpy(out->time, "--:--:--");
    if (!s) return;
    if (s->environment_state == SENSOR_READY && isfinite(s->temperature) && isfinite(s->humidity) &&
        s->temperature >= -40 && s->temperature <= 125 && s->humidity >= 0 && s->humidity <= 100) {
        snprintf(out->temperature, sizeof(out->temperature), "%.1f", (double)s->temperature);
        snprintf(out->humidity, sizeof(out->humidity), "%.1f", (double)s->humidity);
    }
    if (s->clock_state == SENSOR_READY &&
             sensor_date_valid(s->year, s->month, s->day, s->hour, s->minute, s->second)) {
        snprintf(out->date, sizeof(out->date), "%04d.%02d.%02d", s->year, s->month, s->day);
        snprintf(out->time, sizeof(out->time), "%02d:%02d:%02d", s->hour, s->minute, s->second);
    }
}

static sensor_ui_t ui;
static lv_timer_t *timer;

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, bool border)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
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
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_label_set_text(obj, text);
    return obj;
}

static bool set_text(lv_obj_t *label, const char *value)
{
    /* The RLCD flushes full frames: avoid invalidating unchanged labels. */
    if (!strcmp(lv_label_get_text(label), value)) return false;
    lv_label_set_text(label, value);
    return true;
}

static void align_unit(lv_obj_t *value, lv_obj_t *unit)
{
    /* Follow the actual value width and align font baselines, including "--"
     * and boundary values such as -40.0 and 100.0. */
    lv_obj_update_layout(value);
    const lv_font_t *value_font = lv_obj_get_style_text_font(value, 0);
    const lv_font_t *unit_font = lv_obj_get_style_text_font(unit, 0);
    int baseline_offset = value_font->line_height - value_font->base_line -
                          (unit_font->line_height - unit_font->base_line);
    lv_obj_set_pos(unit, lv_obj_get_x(value) + lv_obj_get_width(value) + 6,
                   lv_obj_get_y(value) + baseline_offset);
}

static void poll(lv_timer_t *unused)
{
    (void)unused;
    sensor_snapshot_t snapshot;
    sensor_text_t text;
    sensor_service_get_snapshot(&snapshot);
    sensor_format(&snapshot, &text);
    if (set_text(ui.temperature, text.temperature)) align_unit(ui.temperature, ui.temperature_unit);
    if (set_text(ui.humidity, text.humidity)) align_unit(ui.humidity, ui.humidity_unit);
    set_text(ui.date, text.date); set_text(ui.time, text.time);
}

static void back_event(lv_event_t *event)
{
    (void)event;
    if (ui.on_back) ui.on_back(ui.context);
}

static void deleted(lv_event_t *event)
{
    if (lv_event_get_target(event) != ui.root) return;
    if (timer) { lv_timer_delete(timer); timer = NULL; }
    sensor_service_set_active(false);
    memset(&ui, 0, sizeof(ui));
}

bool init_sensor_ui_screen(lv_obj_t *parent, int32_t x, int32_t y, sensor_back_cb_t back, void *context)
{
    if (!parent) return false;
    deinit_sensor_ui();
    if (!sensor_service_start()) return false;
    ui.on_back = back; ui.context = context;
    ui.root = box(parent, x, y, 400, 300, false);
    lv_obj_set_user_data(ui.root, &ui);
    lv_obj_add_event_cb(ui.root, deleted, LV_EVENT_DELETE, NULL);
    rlcd_app_header_t header = rlcd_app_header_create(ui.root, "Sensor", &rlcd_header_sensor, back_event, NULL);
    ui.back = header.back;

    lv_obj_t *caption = label(ui.root, "Date & time", 12, 61, 376, &lv_font_montserrat_14);
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    ui.time = label(ui.root, "--:--:--", 12, 81, 376, &lv_font_montserrat_48);
    lv_obj_set_style_text_align(ui.time, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(ui.time, -1, 0);
    ui.date = label(ui.root, "----.--.--", 12, 140, 376, &lv_font_montserrat_16);
    lv_obj_set_style_text_align(ui.date, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *temperature = box(ui.root, 12, 181, 183, 107, true);
    label(temperature, "Temperature", 12, 12, 157, &lv_font_montserrat_14);
    ui.temperature = label(temperature, "--", 12, 39, LV_SIZE_CONTENT, &lv_font_montserrat_40);
    lv_obj_set_style_text_letter_space(ui.temperature, -1, 0);
    ui.temperature_unit = label(temperature, "\xC2\xB0" "C", 0, 0, LV_SIZE_CONTENT, &lv_font_montserrat_18);
    align_unit(ui.temperature, ui.temperature_unit);
    lv_obj_t *humidity = box(ui.root, 205, 181, 183, 107, true);
    label(humidity, "Humidity", 12, 12, 157, &lv_font_montserrat_14);
    ui.humidity = label(humidity, "--", 12, 39, LV_SIZE_CONTENT, &lv_font_montserrat_40);
    lv_obj_set_style_text_letter_space(ui.humidity, -1, 0);
    ui.humidity_unit = label(humidity, "%", 0, 0, LV_SIZE_CONTENT, &lv_font_montserrat_18);
    align_unit(ui.humidity, ui.humidity_unit);
    timer = lv_timer_create(poll, 100, NULL);
    if (!timer) { deinit_sensor_ui(); return false; }
    sensor_service_set_active(true);
    poll(NULL);
    return true;
}

void deinit_sensor_ui(void) { if (ui.root) lv_obj_delete(ui.root); }
void pause_sensor_ui(bool paused)
{
    if (!timer) return;
    sensor_service_set_active(!paused);
    if (paused) lv_timer_pause(timer);
    else { poll(NULL); lv_timer_resume(timer); }
}
