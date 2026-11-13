#include "sensor_service.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "sensor";
static portMUX_TYPE snapshot_lock = portMUX_INITIALIZER_UNLOCKED;
static sensor_snapshot_t snapshot;
static TaskHandle_t worker_task;
static bool active;
static uint32_t generation;

void sensor_service_get_snapshot(sensor_snapshot_t *out)
{
    if (!out) return;
    portENTER_CRITICAL(&snapshot_lock);
    *out = snapshot;
    portEXIT_CRITICAL(&snapshot_lock);
}

static bool current(uint32_t session)
{
    portENTER_CRITICAL(&snapshot_lock);
    bool result = active && session == generation;
    portEXIT_CRITICAL(&snapshot_lock);
    return result;
}

static void report_init(const char *name, esp_err_t status)
{
    if (status != ESP_OK) ESP_LOGW(TAG, "%s init failed: %s; showing --", name, esp_err_to_name(status));
}

static void worker(void *context)
{
    (void)context;
    for (;;) {
        portENTER_CRITICAL(&snapshot_lock);
        bool run = active;
        uint32_t session = generation;
        portEXIT_CRITICAL(&snapshot_lock);
        if (!run) { ulTaskNotifyTake(pdTRUE, portMAX_DELAY); continue; }

        sensor_snapshot_t data = {0};
        esp_err_t clock_status = pcf_init();
        report_init("PCF85063A", clock_status);
        if (!current(session)) continue;
        esp_err_t environment_status = shtc3_init();
        report_init("SHTC3", environment_status);
        if (!current(session)) continue;
        data.clock_state = clock_status == ESP_OK ? SENSOR_WAITING : SENSOR_UNAVAILABLE;
        data.environment_state = environment_status == ESP_OK ? SENSOR_WAITING : SENSOR_UNAVAILABLE;
        int64_t next_clock = 0, next_environment = 0;
        esp_err_t last_clock_error = ESP_OK;
        while (current(session)) {
            int64_t now = esp_timer_get_time();
            if (clock_status == ESP_OK && now >= next_clock) {
                bsp_sensor_datetime_t t;
                esp_err_t ret = bsp_sensor_read_datetime(&t);
                if (ret != last_clock_error) {
                    if (ret != ESP_OK) ESP_LOGW(TAG, "RTC read failed: %s", esp_err_to_name(ret));
                    else ESP_LOGI(TAG, "RTC readings recovered");
                    last_clock_error = ret;
                }
                if (ret == ESP_OK) {
                    data.clock_state = SENSOR_READY;
                    data.year = t.year; data.month = t.month; data.day = t.day;
                    data.hour = t.hour; data.minute = t.minute; data.second = t.second;
                }
                next_clock = esp_timer_get_time() + 1000000;
            }
            if (!current(session)) break;
            if (environment_status == ESP_OK && now >= next_environment) {
                esp_err_t ret = bsp_sensor_read_environment(&data.temperature, &data.humidity);
                data.environment_state = ret == ESP_OK ? SENSOR_READY : SENSOR_UNAVAILABLE;
                next_environment = now + 5000000;
            }
            portENTER_CRITICAL(&snapshot_lock);
            if (active && session == generation) snapshot = data;
            portEXIT_CRITICAL(&snapshot_lock);
            /* Close/pause wakes us without deleting a task during an I2C transaction. */
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
        }
    }
}

bool sensor_service_start(void)
{
    if (worker_task) return true;
    return xTaskCreate(worker, "rlcd_sensor", 4096, NULL, 2, &worker_task) == pdPASS;
}

void sensor_service_set_active(bool enabled)
{
    if (!worker_task) return;
    portENTER_CRITICAL(&snapshot_lock);
    if (active != enabled) {
        active = enabled;
        ++generation;
        memset(&snapshot, 0, sizeof(snapshot));
    }
    portEXIT_CRITICAL(&snapshot_lock);
    xTaskNotifyGive(worker_task);
}
