/* SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0 */
#include "example_support.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <stdio.h>

static portMUX_TYPE sample_lock = portMUX_INITIALIZER_UNLOCKED;
static float temperature, humidity;
static bool valid;
static lv_obj_t *temperature_label, *humidity_label;

static void refresh(lv_timer_t *timer)
{
    (void)timer;
    portENTER_CRITICAL(&sample_lock);
    float t = temperature, h = humidity;
    bool available = valid;
    portEXIT_CRITICAL(&sample_lock);
    char temp[24] = "--.-", hum[24] = "--.-";
    if (available) {
        snprintf(temp, sizeof(temp), "%.1f", (double)t);
        snprintf(hum, sizeof(hum), "%.1f", (double)h);
    }
    example_label_update(temperature_label, temp);
    example_label_update(humidity_label, hum);
}

static lv_obj_t *measurement(lv_obj_t *screen, const char *name, const char *unit, int x)
{
    lv_obj_t *card = example_box(screen, x, 72, 176, 196);
    lv_obj_t *caption = example_label(card, name, 4, 19, 168, &lv_font_montserrat_14);
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *value = example_label(card, "--.-", 2, 68, 172, &lv_font_montserrat_40);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *suffix = example_label(card, unit, 4, 143, 168, &lv_font_montserrat_20);
    lv_obj_set_style_text_align(suffix, LV_TEXT_ALIGN_CENTER, 0);
    return value;
}

void app_main(void)
{
    lv_obj_t *screen = example_screen_begin("Room climate");
    temperature_label = measurement(screen, "TEMPERATURE", "\xc2\xb0" "C", 16);
    humidity_label = measurement(screen, "HUMIDITY", "% RH", 208);
    lv_timer_create(refresh, 250, NULL);
    bsp_display_unlock();
    bool initialized = false;
    unsigned failures = 0;
    for (;;) {
        esp_err_t err = initialized ? ESP_OK : shtc3_init();
        initialized = err == ESP_OK;
        float t = 0, h = 0;
        if (initialized) err = bsp_sensor_read_environment(&t, &h);
        portENTER_CRITICAL(&sample_lock);
        valid = err == ESP_OK;
        if (valid) { temperature = t; humidity = h; }
        portEXIT_CRITICAL(&sample_lock);
        if (err != ESP_OK) {
            if (failures++ % 10 == 0) ESP_LOGW("climate", "SHTC3: %s", esp_err_to_name(err));
            initialized = false;
        } else failures = 0;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
