#include <stdio.h>
#include <limits.h>
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "driver/gpio.h"
#include "driver/i2s_tdm.h"

#include "esp_lcd_touch_cst816s.h"
#include "esp_rlcd_st7305.h"
#include "esp_codec_dev_defaults.h"
#include "pcf85063a.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "bsp_button_debounce.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "bsp/display.h"
#include "bsp/touch.h"
#include "bsp/esp32_s3_touch_rlcd_4_2b.h"
#include "bsp_err_check.h"

static const char *TAG = "ESP32-S3-Touch-RLCD-4.2B";

static esp_timer_handle_t user_button_timer;
static bsp_button_debounce_t user_button_state;
static uint32_t user_button_presses;
static bool boot_button_initialized;
static portMUX_TYPE user_button_lock = portMUX_INITIALIZER_UNLOCKED;

static void user_button_poll(void *unused)
{
    (void)unused;
    bool pressed = gpio_get_level(BSP_USER_BUTTON_IO) == BSP_USER_BUTTON_ACTIVE_LEVEL;
    if (bsp_button_debounce_update(&user_button_state, pressed, (uint32_t)(esp_timer_get_time() / 1000))) {
        portENTER_CRITICAL(&user_button_lock);
        ++user_button_presses;
        portEXIT_CRITICAL(&user_button_lock);
    }
}

esp_err_t bsp_user_button_init(void)
{
    if (user_button_timer) return ESP_OK;
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << BSP_USER_BUTTON_IO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = BSP_USER_BUTTON_ACTIVE_LEVEL == 0 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = BSP_USER_BUTTON_ACTIVE_LEVEL == 1 ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&config);
    if (ret != ESP_OK) return ret;
    bsp_button_debounce_init(&user_button_state,
        gpio_get_level(BSP_USER_BUTTON_IO) == BSP_USER_BUTTON_ACTIVE_LEVEL,
        (uint32_t)(esp_timer_get_time() / 1000));
    const esp_timer_create_args_t timer = {
        .callback = user_button_poll, .name = "bsp_user_key", .skip_unhandled_events = true,
    };
    ret = esp_timer_create(&timer, &user_button_timer);
    if (ret == ESP_OK) ret = esp_timer_start_periodic(user_button_timer, 10000);
    if (ret != ESP_OK && user_button_timer) {
        esp_timer_delete(user_button_timer);
        user_button_timer = NULL;
    }
    return ret;
}

uint32_t bsp_user_button_get_press_count(void)
{
    portENTER_CRITICAL(&user_button_lock);
    uint32_t count = user_button_presses;
    portEXIT_CRITICAL(&user_button_lock);
    return count;
}

esp_err_t bsp_button_test_init(void)
{
    esp_err_t ret = bsp_user_button_init();
    if (ret != ESP_OK || boot_button_initialized) return ret;
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << BSP_BOOT_BUTTON_IO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&config);
    if (ret == ESP_OK) boot_button_initialized = true;
    return ret;
}

esp_err_t bsp_button_get_pressed(bsp_button_id_t id, bool *pressed)
{
    if (!pressed) return ESP_ERR_INVALID_ARG;
    switch (id) {
    case BSP_BUTTON_ID_BOOT:
        if (!boot_button_initialized) return ESP_ERR_INVALID_STATE;
        *pressed = gpio_get_level(BSP_BOOT_BUTTON_IO) == BSP_BOOT_BUTTON_ACTIVE_LEVEL;
        return ESP_OK;
    case BSP_BUTTON_ID_USER:
        if (!user_button_timer) return ESP_ERR_INVALID_STATE;
        *pressed = gpio_get_level(BSP_USER_BUTTON_IO) == BSP_USER_BUTTON_ACTIVE_LEVEL;
        return ESP_OK;
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

/**************************************************************************************************
 * Battery: GPIO4 / ADC1 CH3, VBAT -> 200k -> ADC -> 100k -> GND
 **************************************************************************************************/
#define BSP_BATTERY_ADC_CHANNEL ADC_CHANNEL_3
#define BSP_BATTERY_SAMPLE_US 500000
#define BSP_BATTERY_SAMPLE_COUNT 10
#define BSP_BATTERY_STALE_US (15LL * 1000000)

static adc_oneshot_unit_handle_t battery_adc;
static adc_cali_handle_t battery_calibration;
static esp_timer_handle_t battery_timer;
static portMUX_TYPE battery_lock = portMUX_INITIALIZER_UNLOCKED;
static bsp_battery_info_t battery_cache;
static int64_t battery_updated_us;
/* Only the timer callback accesses the accumulator and filter. */
static int battery_sum_mv, battery_samples, battery_filtered_q4;
static bool battery_read_error;

static int battery_estimate_percent(int voltage_mv)
{
    /* Same voltage-based estimate as the Xiaozhi RLCD-4.2 board.
     * This is not a coulomb counter; load/charging affects the estimate. */
    int64_t v = voltage_mv;
    int percent = (int)((-v * v + 9016 * v - 19189000) / 10000);
    return percent < 0 ? 0 : percent > 100 ? 100 : percent;
}

static void battery_poll(void *unused)
{
    (void)unused;
    int raw = 0, adc_mv = 0;
    esp_err_t ret = adc_oneshot_read(battery_adc, BSP_BATTERY_ADC_CHANNEL, &raw);
    if (ret == ESP_OK && (raw <= 0 || raw >= 4095)) ret = ESP_ERR_INVALID_RESPONSE;
    if (ret == ESP_OK) ret = adc_cali_raw_to_voltage(battery_calibration, raw, &adc_mv);
    int voltage_mv = adc_mv * 3;
    /* Reject invalid electrical readings, not ordinary low battery voltage.
     * This range cannot identify an absent battery when USB is connected. */
    if (ret == ESP_OK && (voltage_mv < 2500 || voltage_mv > 4500)) ret = ESP_ERR_INVALID_RESPONSE;
    if (ret != ESP_OK) {
        battery_sum_mv = battery_samples = 0;
        if (!battery_read_error) ESP_LOGW(TAG, "Battery sample failed: %s", esp_err_to_name(ret));
        battery_read_error = true;
        return;
    }

    battery_sum_mv += voltage_mv;
    if (++battery_samples < BSP_BATTERY_SAMPLE_COUNT) return;
    voltage_mv = (battery_sum_mv + BSP_BATTERY_SAMPLE_COUNT / 2) / BSP_BATTERY_SAMPLE_COUNT;
    battery_sum_mv = battery_samples = 0;
    battery_read_error = false;
    int64_t now = esp_timer_get_time();
    bool first = battery_updated_us == 0 || now - battery_updated_us > BSP_BATTERY_STALE_US;
    if (first) battery_filtered_q4 = voltage_mv * 16;
    else battery_filtered_q4 += (voltage_mv * 16 - battery_filtered_q4) / 4;
    bsp_battery_info_t info = {
        .voltage_mv = (battery_filtered_q4 + 8) / 16,
    };
    info.percent = battery_estimate_percent(info.voltage_mv);
    portENTER_CRITICAL(&battery_lock);
    battery_cache = info;
    battery_updated_us = now;
    portEXIT_CRITICAL(&battery_lock);
    if (first) ESP_LOGI(TAG, "Battery: %d mV, estimated %d%%", info.voltage_mv, info.percent);
}

esp_err_t bsp_battery_init(void)
{
    /* Startup-only initialization, idempotent after success. No extra task stack.
     * The shared timer performs one short ADC conversion per callback, without
     * sleeping or retrying; UI readers only access the cached result. */
    if (battery_timer) return ESP_OK;
    const adc_oneshot_unit_init_cfg_t unit = {
        .unit_id = ADC_UNIT_1,
    };
    const adc_oneshot_chan_cfg_t channel = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    const adc_cali_curve_fitting_config_t calibration = {
        .unit_id = ADC_UNIT_1,
        .chan = BSP_BATTERY_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    const esp_timer_create_args_t timer = {
        .callback = battery_poll, .name = "bsp_battery", .skip_unhandled_events = true,
    };
    esp_err_t ret = adc_oneshot_new_unit(&unit, &battery_adc);
    if (ret == ESP_OK) ret = adc_oneshot_config_channel(battery_adc, BSP_BATTERY_ADC_CHANNEL, &channel);
    if (ret == ESP_OK) ret = adc_cali_create_scheme_curve_fitting(&calibration, &battery_calibration);
    if (ret == ESP_OK) ret = esp_timer_create(&timer, &battery_timer);
    if (ret == ESP_OK) ret = esp_timer_start_periodic(battery_timer, BSP_BATTERY_SAMPLE_US);
    if (ret != ESP_OK) {
        if (battery_timer) esp_timer_delete(battery_timer);
        if (battery_calibration) adc_cali_delete_scheme_curve_fitting(battery_calibration);
        if (battery_adc) adc_oneshot_del_unit(battery_adc);
        battery_timer = NULL;
        battery_calibration = NULL;
        battery_adc = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "Battery ADC ready: GPIO%d, divider x3, average every 5 s", BSP_BATTERY_ADC_IO);
    return ESP_OK;
}

esp_err_t bsp_battery_get_info(bsp_battery_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&battery_lock);
    *info = battery_cache;
    int64_t updated = battery_updated_us;
    portEXIT_CRITICAL(&battery_lock);
    esp_err_t ret = updated == 0 ? ESP_ERR_INVALID_STATE :
        esp_timer_get_time() - updated > BSP_BATTERY_STALE_US ? ESP_ERR_TIMEOUT : ESP_OK;
    if (ret != ESP_OK) {
        info->voltage_mv = 0;
        info->percent = -1;
    }
    return ret;
}

static i2c_master_bus_handle_t i2c_handle = NULL;
static bool i2c_initialized = false;
static lv_display_t *disp = NULL;
static lv_indev_t *disp_indev = NULL;
sdmmc_card_t *bsp_sdcard = NULL;
static esp_lcd_touch_handle_t tp = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static uint8_t lvgl_i1_fb[BSP_LCD_H_RES * BSP_LCD_V_RES / 8 + 8];
static uint8_t st7305_tx_fb[BSP_LCD_H_RES * BSP_LCD_V_RES / 8];
static uint8_t st7305_pair_lut[4][4][256];
static uint16_t st7305_src_byte_x[BSP_LCD_H_RES / 2];
static uint16_t st7305_src_row_offset[BSP_LCD_V_RES / 4][4];
static bool st7305_lut_initialized = false;
static i2s_chan_handle_t i2s_tx_chan = NULL;
static i2s_chan_handle_t i2s_rx_chan = NULL;
static const audio_codec_data_if_t *i2s_data_if = NULL;
static esp_codec_dev_handle_t speaker_codec;
static esp_codec_dev_handle_t microphone_codec;
static bool voice_output_open, voice_input_open;
static bsp_audio_owner_t audio_owner;
static portMUX_TYPE audio_owner_lock = portMUX_INITIALIZER_UNLOCKED;
static bool playback_open;
static bool playback_muted = true;
static int playback_volume = -1;
static SemaphoreHandle_t playback_lock;
#define BSP_AUDIO_DMA_DESCRIPTORS 6
#define BSP_AUDIO_DMA_FRAMES 240

#define BSP_ES7210_CODEC_ADDR ES7210_CODEC_DEFAULT_ADDR
#define BSP_I2S_GPIO_CFG       \
    {                          \
        .mclk = BSP_I2S_MCLK,  \
        .bclk = BSP_I2S_SCLK,  \
        .ws = BSP_I2S_LCLK,    \
        .dout = BSP_I2S_DOUT,  \
        .din = BSP_I2S_DSIN,   \
        .invert_flags = {      \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv = false,   \
        },                     \
    }

#define BSP_I2S_DUPLEX_MONO_CFG(_sample_rate)                                                         \
    {                                                                                                 \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                          \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                 \
    }

esp_err_t bsp_i2c_init(void)
{
    if (i2c_initialized) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .i2c_port = BSP_I2C_NUM,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
        .trans_queue_depth = 0,
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_new_master_bus(&i2c_bus_conf, &i2c_handle));

    i2c_initialized = true;
    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(i2c_del_master_bus(i2c_handle));
    i2c_handle = NULL;
    i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    bsp_i2c_init();
    return i2c_handle;
}

/**************************************************************************************************
 * RTC and temperature/humidity
 **************************************************************************************************/
static StaticSemaphore_t sensor_mutex_storage;
static SemaphoreHandle_t sensor_mutex;
static portMUX_TYPE sensor_create_lock = portMUX_INITIALIZER_UNLOCKED;
static pcf85063a_dev_t rtc;
static i2c_master_dev_handle_t shtc3;
static bool rtc_ready, shtc3_ready;
/* Same factory-test start time as the C5-2.8 BSP. */
static const pcf85063a_datetime_t Set_Time = {
    .year = 2026, .month = 4, .day = 22, .dotw = 1,
    .hour = 23, .min = 59, .sec = 0
};

static void sensor_take_lock(void)
{
    portENTER_CRITICAL(&sensor_create_lock);
    if (!sensor_mutex) sensor_mutex = xSemaphoreCreateMutexStatic(&sensor_mutex_storage);
    portEXIT_CRITICAL(&sensor_create_lock);
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
}

static void sensor_drop_device(i2c_master_dev_handle_t *device)
{
    if (*device && i2c_master_bus_rm_device(*device) == ESP_OK) *device = NULL;
}

static uint8_t sensor_crc8(const uint8_t *bytes, size_t length)
{
    uint8_t crc = 0xff;
    for (size_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint8_t)((crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1);
    }
    return crc;
}

static bool sensor_decode_environment(const uint8_t raw[6], float *temperature, float *humidity)
{
    if (!raw || !temperature || !humidity || sensor_crc8(raw, 2) != raw[2] ||
        sensor_crc8(raw + 3, 2) != raw[5]) return false;
    float t = -45.0f + 175.0f * ((raw[0] << 8) | raw[1]) / 65536.0f;
    float h = 100.0f * ((raw[3] << 8) | raw[4]) / 65536.0f;
    if (t < -40 || t > 125 || h < 0 || h > 100) return false;
    *temperature = t; *humidity = h;
    return true;
}

static bool sensor_datetime_valid(const bsp_sensor_datetime_t *t)
{
    static const unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (!t || t->year < 1970 || t->year > 2069 || t->month < 1 || t->month > 12 ||
        t->hour < 0 || t->hour > 23 || t->minute < 0 || t->minute > 59 ||
        t->second < 0 || t->second > 59 || t->weekday < 0 || t->weekday > 6) return false;
    bool leap = t->year % 4 == 0 && (t->year % 100 != 0 || t->year % 400 == 0);
    unsigned limit = days[t->month - 1] + (t->month == 2 && leap);
    return t->day > 0 && (unsigned)t->day <= limit;
}

static esp_err_t shtc3_command(uint16_t command)
{
    uint8_t bytes[] = {(uint8_t)(command >> 8), (uint8_t)command};
    return i2c_master_transmit(shtc3, bytes, sizeof(bytes), 100);
}

static esp_err_t shtc3_wake(void)
{
    esp_err_t ret = shtc3_command(0x3517);
    if (ret == ESP_OK) esp_rom_delay_us(240);
    return ret;
}

esp_err_t pcf_init(void)
{
    sensor_take_lock();
    esp_err_t ret = ESP_OK;
    rtc_ready = false;
    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (!bus) { ret = ESP_ERR_INVALID_STATE; goto done; }
    sensor_drop_device(&rtc.dev_handle);
    if (rtc.dev_handle) { ret = ESP_ERR_INVALID_STATE; goto done; }
    ret = pcf85063a_init(&rtc, bus, PCF85063A_ADDRESS);
    if (ret == ESP_OK) ret = pcf85063a_set_time_date(&rtc, Set_Time);
    if (ret == ESP_OK) {
        rtc_ready = true;
        ESP_LOGI(TAG, "pcf85063a init success (C5 start time: 2026-04-22 23:59:00)");
    } else sensor_drop_device(&rtc.dev_handle);
done:
    xSemaphoreGive(sensor_mutex);
    return ret;
}

esp_err_t shtc3_init(void)
{
    sensor_take_lock();
    esp_err_t ret = ESP_OK;
    if (shtc3_ready) goto done;
    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (!bus) { ret = ESP_ERR_INVALID_STATE; goto done; }
    ret = i2c_master_probe(bus, 0x70, 50);
    if (ret != ESP_OK) goto done;
    if (!shtc3) {
        const i2c_device_config_t config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = 0x70, .scl_speed_hz = 400000
        };
        ret = i2c_master_bus_add_device(bus, &config, &shtc3);
    }
    if (ret == ESP_OK) ret = shtc3_wake();
    uint8_t command[] = {0xef, 0xc8}, id[3];
    if (ret == ESP_OK) ret = i2c_master_transmit_receive(shtc3, command, 2, id, 3, 100);
    if (ret == ESP_OK && (sensor_crc8(id, 2) != id[2] ||
        ((((uint16_t)id[0] << 8) | id[1]) & 0x083f) != 0x0807)) ret = ESP_ERR_INVALID_RESPONSE;
    if (shtc3) {
        esp_err_t sleep_ret = shtc3_command(0xb098);
        if (ret == ESP_OK) ret = sleep_ret;
    }
    if (ret == ESP_OK) {
        shtc3_ready = true;
        ESP_LOGI(TAG, "SHTC3 ready at 0x70");
    } else sensor_drop_device(&shtc3);
done:
    xSemaphoreGive(sensor_mutex);
    return ret;
}

esp_err_t bsp_sensor_read_environment(float *temperature, float *humidity)
{
    if (!temperature || !humidity) return ESP_ERR_INVALID_ARG;
    sensor_take_lock();
    esp_err_t ret = shtc3_ready ? shtc3_wake() : ESP_ERR_INVALID_STATE;
    /* Normal-precision measurement without clock stretching: touch can use
     * the shared I2C bus while the SHTC3 converts the sample. */
    if (ret == ESP_OK) ret = shtc3_command(0x7866);
    uint8_t raw[6];
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(15) + 1);
        ret = i2c_master_receive(shtc3, raw, sizeof(raw), 100);
        if (ret == ESP_OK && !sensor_decode_environment(raw, temperature, humidity))
            ret = ESP_ERR_INVALID_RESPONSE;
    }
    if (shtc3_ready) {
        esp_err_t sleep_ret = shtc3_command(0xb098);
        if (ret == ESP_OK) ret = sleep_ret;
    }
    xSemaphoreGive(sensor_mutex);
    return ret;
}

esp_err_t bsp_sensor_read_datetime(bsp_sensor_datetime_t *datetime)
{
    if (!datetime) return ESP_ERR_INVALID_ARG;
    sensor_take_lock();
    pcf85063a_datetime_t now;
    esp_err_t ret = rtc_ready ?
        pcf85063a_get_time_date(&rtc, &now) : ESP_ERR_INVALID_STATE;
    if (ret == ESP_OK) {
        *datetime = (bsp_sensor_datetime_t){
            .year = now.year, .month = now.month, .day = now.day, .weekday = now.dotw,
            .hour = now.hour, .minute = now.min, .second = now.sec
        };
    }
    xSemaphoreGive(sensor_mutex);
    return ret;
}

esp_err_t bsp_sensor_set_datetime(const bsp_sensor_datetime_t *t)
{
    if (!sensor_datetime_valid(t)) return ESP_ERR_INVALID_ARG;
    sensor_take_lock();
    pcf85063a_datetime_t value = {
        .year = t->year, .month = t->month, .day = t->day, .dotw = t->weekday,
        .hour = t->hour, .min = t->minute, .sec = t->second
    };
    esp_err_t ret = rtc_ready ? pcf85063a_set_time_date(&rtc, value) : ESP_ERR_INVALID_STATE;
    xSemaphoreGive(sensor_mutex);
    return ret;
}

/**************************************************************************************************
 * Storage
 **************************************************************************************************/
esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
#ifdef CONFIG_BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);
    BSP_ERROR_CHECK_RETURN_ERR(ret_val);

    size_t total = 0;
    size_t used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret_val;
}

esp_err_t bsp_spiffs_unmount(void)
{
    return esp_vfs_spiffs_unregister(CONFIG_BSP_SPIFFS_PARTITION_LABEL);
}

static StaticSemaphore_t sdcard_mutex_storage;
static SemaphoreHandle_t sdcard_mutex;
static unsigned sdcard_readers;
static portMUX_TYPE sdcard_create_lock = portMUX_INITIALIZER_UNLOCKED;

static void sdcard_take_lock(void)
{
    portENTER_CRITICAL(&sdcard_create_lock);
    if (!sdcard_mutex) sdcard_mutex = xSemaphoreCreateMutexStatic(&sdcard_mutex_storage);
    portEXIT_CRITICAL(&sdcard_create_lock);
    xSemaphoreTake(sdcard_mutex, portMAX_DELAY);
}

static esp_err_t sdcard_mount_locked(void)
{
    if (bsp_sdcard != NULL) {
        return ESP_OK;
    }

    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_BSP_SD_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    const sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    const sdmmc_slot_config_t slot_config = {
        .clk = BSP_SD_CLK,
        .cmd = BSP_SD_CMD,
        .d0 = BSP_SD_D0,
        .d1 = GPIO_NUM_NC,
        .d2 = GPIO_NUM_NC,
        .d3 = GPIO_NUM_NC,
        .d4 = GPIO_NUM_NC,
        .d5 = GPIO_NUM_NC,
        .d6 = GPIO_NUM_NC,
        .d7 = GPIO_NUM_NC,
        .cd = SDMMC_SLOT_NO_CD,
        .wp = SDMMC_SLOT_NO_WP,
        .width = 1,
        .flags = 0,
    };

#if defined(CONFIG_FATFS_LFN_NONE) && CONFIG_FATFS_LFN_NONE
    ESP_LOGW(TAG, "Warning: Long filenames on SD card are disabled in menuconfig!");
#endif

    ESP_LOGI(TAG, "SDMMC 1-bit: CMD=%d, CLK=%d, D0=%d, no CS",
             BSP_SD_CMD, BSP_SD_CLK, BSP_SD_D0);
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, &host, &slot_config,
                                          &mount_config, &bsp_sdcard);
    if (ret != ESP_OK) {
        bsp_sdcard = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted at %s", BSP_SD_MOUNT_POINT);
    return ESP_OK;
}

esp_err_t bsp_sdcard_mount(void)
{
    sdcard_take_lock();
    esp_err_t ret = sdcard_mount_locked();
    xSemaphoreGive(sdcard_mutex);
    return ret;
}

esp_err_t bsp_sdcard_unmount(void)
{
    sdcard_take_lock();
    if (sdcard_readers) { xSemaphoreGive(sdcard_mutex); return ESP_ERR_INVALID_STATE; }
    esp_err_t ret = ESP_OK;
    if (bsp_sdcard != NULL) {
        ret = esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, bsp_sdcard);
        if (ret == ESP_OK) bsp_sdcard = NULL;
        else ESP_LOGW(TAG, "SD card unmount failed: %s", esp_err_to_name(ret));
    }
    xSemaphoreGive(sdcard_mutex);
    return ret;
}

esp_err_t bsp_sdcard_acquire(void)
{
    sdcard_take_lock();
    esp_err_t ret = sdcard_mount_locked();
    if (ret == ESP_OK) ++sdcard_readers;
    xSemaphoreGive(sdcard_mutex);
    return ret;
}

void bsp_sdcard_release(void)
{
    sdcard_take_lock();
    if (sdcard_readers) --sdcard_readers;
    xSemaphoreGive(sdcard_mutex);
}

esp_err_t bsp_sdcard_get_info(bsp_sdcard_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    *info = (bsp_sdcard_info_t){0};
    sdcard_take_lock();
    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (bsp_sdcard) {
        info->mounted = true;
        info->capacity_bytes = (uint64_t)bsp_sdcard->csd.capacity * bsp_sdcard->csd.sector_size;
        ret = esp_vfs_fat_info(BSP_SD_MOUNT_POINT, &info->total_bytes, &info->free_bytes);
    }
    xSemaphoreGive(sdcard_mutex);
    return ret;
}

esp_err_t bsp_audio_poweramp_enable(bool enable)
{
    static bool configured;
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(BSP_POWER_AMP_IO),
        .mode = GPIO_MODE_OUTPUT,
    };
    if (!configured) {
        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) return err;
        configured = true;
    }
    return gpio_set_level(BSP_POWER_AMP_IO, enable);
}

esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config)
{
    esp_err_t ret = ESP_FAIL;
    if (i2s_tx_chan && i2s_rx_chan) {
        return ESP_OK;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = BSP_AUDIO_DMA_DESCRIPTORS;
    chan_cfg.dma_frame_num = BSP_AUDIO_DMA_FRAMES;
    chan_cfg.auto_clear = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &i2s_tx_chan, &i2s_rx_chan), TAG, "Create audio channels");

    const i2s_std_config_t std_cfg_default = BSP_I2S_DUPLEX_MONO_CFG(22050);
    const i2s_std_config_t *p_i2s_cfg = i2s_config ? i2s_config : &std_cfg_default;
    i2s_std_config_t tx_config = *p_i2s_cfg;
    tx_config.gpio_cfg.din = I2S_GPIO_UNUSED;
    /* Same clocks and TDM wiring as upstream RLCD BoxAudioCodec and 2.06.
     * Keep RX in TDM mode across Music/Voice sessions; codec open selects the
     * current rate/slots. Music's output remains 24 kHz mono. */
    i2s_tdm_config_t rx_config = {
        .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(p_i2s_cfg->clk_cfg.sample_rate_hz),
        .slot_cfg = I2S_TDM_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_STEREO, I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };
    rx_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    rx_config.clk_cfg.bclk_div = 8;
    rx_config.slot_cfg.total_slot = 4;
    rx_config.gpio_cfg.dout = I2S_GPIO_UNUSED;

    if (i2s_tx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_chan, &tx_config), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_tx_chan), err, TAG, "I2S enabling failed");
    }
    if (i2s_rx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_tdm_mode(i2s_rx_chan, &rx_config), err, TAG, "I2S TDM initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_rx_chan), err, TAG, "I2S enabling failed");
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = CONFIG_BSP_I2S_NUM,
        .rx_handle = i2s_rx_chan,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (!i2s_data_if) { ret = ESP_ERR_NO_MEM; goto err; }

    ESP_GOTO_ON_ERROR(bsp_audio_poweramp_enable(false), err, TAG, "Mute amplifier");
    return ESP_OK;

err:
    if (i2s_data_if) { audio_codec_delete_data_if(i2s_data_if); i2s_data_if = NULL; }
    if (i2s_tx_chan) {
        i2s_channel_disable(i2s_tx_chan);
        i2s_del_channel(i2s_tx_chan);
        i2s_tx_chan = NULL;
    }
    if (i2s_rx_chan) {
        i2s_channel_disable(i2s_rx_chan);
        i2s_del_channel(i2s_rx_chan);
        i2s_rx_chan = NULL;
    }
    return ret;
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    if (speaker_codec) return speaker_codec;
    if (bsp_i2c_init() != ESP_OK) return NULL;
    if (i2c_master_probe(i2c_handle, ES8311_CODEC_DEFAULT_ADDR >> 1, 100) != ESP_OK) {
        ESP_LOGE(TAG, "ES8311 not found at 0x%02x", ES8311_CODEC_DEFAULT_ADDR >> 1);
        return NULL;
    }
    if (i2s_data_if == NULL) {
        if (bsp_audio_init(NULL) != ESP_OK) return NULL;
    }

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (!gpio_if || !i2c_ctrl_if) {
        if (gpio_if) audio_codec_delete_gpio_if(gpio_if);
        if (i2c_ctrl_if) audio_codec_delete_ctrl_if(i2c_ctrl_if);
        return NULL;
    }

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = GPIO_NUM_NC,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };
    const audio_codec_if_t *es8311_dev = es8311_codec_new(&es8311_cfg);
    if (!es8311_dev) {
        audio_codec_delete_ctrl_if(i2c_ctrl_if); audio_codec_delete_gpio_if(gpio_if);
        return NULL;
    }

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es8311_dev,
        .data_if = i2s_data_if,
    };
    speaker_codec = esp_codec_dev_new(&codec_dev_cfg);
    if (!speaker_codec) {
        audio_codec_delete_codec_if(es8311_dev);
        audio_codec_delete_ctrl_if(i2c_ctrl_if); audio_codec_delete_gpio_if(gpio_if);
    }
    return speaker_codec;
}

/* Called with playback_lock held. UI changes only the requested value; codec
 * I/O stays on the audio tasks and cannot race with pause or close. */
static esp_err_t playback_update_volume(bool force)
{
    int volume = bsp_audio_volume_get();
    if (!force && volume == playback_volume) return ESP_OK;
    bool muted = playback_muted || volume == 0;
    if (muted && bsp_audio_poweramp_enable(false) != ESP_OK) return ESP_FAIL;
    if (volume != playback_volume && esp_codec_dev_set_out_vol(speaker_codec, volume) != ESP_CODEC_DEV_OK)
        return ESP_FAIL;
    if (esp_codec_dev_set_out_mute(speaker_codec, muted) != ESP_CODEC_DEV_OK) return ESP_FAIL;
    if (!muted && bsp_audio_poweramp_enable(true) != ESP_OK) return ESP_FAIL;
    playback_volume = volume;
    return ESP_OK;
}

esp_err_t bsp_audio_playback_open(void)
{
    /* Open/close have one owner (Music worker); the decoder only writes PCM. */
    if (bsp_audio_session_get_owner() != BSP_AUDIO_OWNER_MUSIC &&
            bsp_audio_session_acquire(BSP_AUDIO_OWNER_MUSIC) != ESP_OK) return ESP_ERR_INVALID_STATE;
    if (!playback_lock) playback_lock = xSemaphoreCreateMutex();
    if (!playback_lock) { bsp_audio_session_release(BSP_AUDIO_OWNER_MUSIC); return ESP_ERR_NO_MEM; }
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (playback_open) goto done;
    if (!bsp_audio_codec_speaker_init()) { ret = ESP_FAIL; goto done; }
    esp_codec_dev_sample_info_t format = {
        .sample_rate = BSP_AUDIO_PLAYBACK_RATE,
        .channel = BSP_AUDIO_PLAYBACK_CHANNELS,
        .bits_per_sample = BSP_AUDIO_PLAYBACK_BITS,
    };
    if (esp_codec_dev_open(speaker_codec, &format) != ESP_CODEC_DEV_OK) { ret = ESP_FAIL; goto done; }
    playback_open = true;
    playback_muted = true;
    playback_volume = -1;
    ret = playback_update_volume(true);
    if (ret != ESP_OK) {
        bsp_audio_poweramp_enable(false);
        esp_codec_dev_close(speaker_codec);
        playback_open = false;
    }
done:
    xSemaphoreGive(playback_lock);
    if (ret != ESP_OK) bsp_audio_session_release(BSP_AUDIO_OWNER_MUSIC);
    return ret;
}

esp_err_t bsp_audio_playback_write(void *pcm, size_t bytes)
{
    if (bsp_audio_session_get_owner() != BSP_AUDIO_OWNER_MUSIC || !playback_lock || !pcm || bytes > INT_MAX) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    esp_err_t ret = playback_open ? playback_update_volume(false) : ESP_ERR_INVALID_STATE;
    if (ret == ESP_OK && esp_codec_dev_write(speaker_codec, pcm, (int)bytes) != ESP_CODEC_DEV_OK) ret = ESP_FAIL;
    xSemaphoreGive(playback_lock);
    return ret;
}

void bsp_audio_playback_drain(void)
{
    if (bsp_audio_session_get_owner() != BSP_AUDIO_OWNER_MUSIC || !playback_open) return;
    /* IDF exposes no TX-drain primitive. With the producer stopped, one complete
     * DMA ring plus a descriptor is an upper bound; auto_clear outputs silence. */
    const unsigned ms = ((BSP_AUDIO_DMA_DESCRIPTORS + 1) * BSP_AUDIO_DMA_FRAMES * 1000 +
                         BSP_AUDIO_PLAYBACK_RATE - 1) / BSP_AUDIO_PLAYBACK_RATE;
    vTaskDelay(pdMS_TO_TICKS(ms));
}

esp_err_t bsp_audio_playback_mute(bool mute)
{
    if (bsp_audio_session_get_owner() != BSP_AUDIO_OWNER_MUSIC) return mute ? ESP_OK : ESP_ERR_INVALID_STATE;
    if (!playback_lock) return mute ? ESP_OK : ESP_ERR_INVALID_STATE;
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    playback_muted = mute;
    esp_err_t ret = playback_open ? playback_update_volume(true) : (mute ? ESP_OK : ESP_ERR_INVALID_STATE);
    xSemaphoreGive(playback_lock);
    return ret;
}

esp_err_t bsp_audio_playback_close(void)
{
    if (bsp_audio_session_get_owner() != BSP_AUDIO_OWNER_MUSIC) return ESP_OK;
    if (!playback_lock) return ESP_OK;
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    esp_err_t ret = ESP_OK;
    if (playback_open) {
        playback_muted = true;
        ret = playback_update_volume(true);
        if (esp_codec_dev_close(speaker_codec) != ESP_CODEC_DEV_OK) ret = ESP_FAIL;
        playback_open = false;
        playback_volume = -1;
    }
    xSemaphoreGive(playback_lock);
    bsp_audio_session_release(BSP_AUDIO_OWNER_MUSIC);
    return ret;
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    if (microphone_codec) return microphone_codec;
    if (bsp_i2c_init() != ESP_OK) return NULL;
    if (i2c_master_probe(i2c_handle, BSP_ES7210_CODEC_ADDR >> 1, 100) != ESP_OK) {
        ESP_LOGE(TAG, "ES7210 not found at 0x%02x", BSP_ES7210_CODEC_ADDR >> 1);
        return NULL;
    }
    if (i2s_data_if == NULL) {
        if (bsp_audio_init(NULL) != ESP_OK) return NULL;
    }
    assert(i2s_data_if);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = BSP_ES7210_CODEC_ADDR,
        .bus_handle = i2c_handle,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    BSP_NULL_CHECK(i2c_ctrl_if, NULL);

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2 | ES7210_SEL_MIC3,
    };
    const audio_codec_if_t *es7210_dev = es7210_codec_new(&es7210_cfg);
    if (!es7210_dev) { audio_codec_delete_ctrl_if(i2c_ctrl_if); return NULL; }

    esp_codec_dev_cfg_t codec_es7210_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = es7210_dev,
        .data_if = i2s_data_if,
    };
    microphone_codec = esp_codec_dev_new(&codec_es7210_dev_cfg);
    if (!microphone_codec) {
        audio_codec_delete_codec_if(es7210_dev); audio_codec_delete_ctrl_if(i2c_ctrl_if);
    }
    return microphone_codec;
}

bsp_audio_owner_t bsp_audio_session_get_owner(void)
{
    portENTER_CRITICAL(&audio_owner_lock);
    bsp_audio_owner_t result = audio_owner;
    portEXIT_CRITICAL(&audio_owner_lock);
    return result;
}
esp_err_t bsp_audio_session_acquire(bsp_audio_owner_t owner)
{
    if (owner != BSP_AUDIO_OWNER_MUSIC && owner != BSP_AUDIO_OWNER_XIAOZHI && owner != BSP_AUDIO_OWNER_MIC_TEST)
        return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&audio_owner_lock);
    esp_err_t result = audio_owner == BSP_AUDIO_OWNER_NONE ? ESP_OK : ESP_ERR_INVALID_STATE;
    if (result == ESP_OK) audio_owner = owner;
    portEXIT_CRITICAL(&audio_owner_lock);
    return result;
}
esp_err_t bsp_audio_session_release(bsp_audio_owner_t owner)
{
    if (owner != BSP_AUDIO_OWNER_MUSIC && owner != BSP_AUDIO_OWNER_XIAOZHI && owner != BSP_AUDIO_OWNER_MIC_TEST)
        return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&audio_owner_lock);
    esp_err_t result = audio_owner == owner ? ESP_OK : ESP_ERR_INVALID_STATE;
    if (result == ESP_OK) audio_owner = BSP_AUDIO_OWNER_NONE;
    portEXIT_CRITICAL(&audio_owner_lock);
    return result;
}
const char *bsp_audio_owner_name(bsp_audio_owner_t owner)
{
    return owner == BSP_AUDIO_OWNER_MUSIC ? "Music" : owner == BSP_AUDIO_OWNER_XIAOZHI ? "Xiaozhi" :
           owner == BSP_AUDIO_OWNER_MIC_TEST ? "Mic Test" : "None";
}
static bool voice_session_owned(void)
{
    bsp_audio_owner_t owner = bsp_audio_session_get_owner();
    return owner == BSP_AUDIO_OWNER_XIAOZHI || owner == BSP_AUDIO_OWNER_MIC_TEST;
}
esp_err_t bsp_audio_voice_open(uint32_t sample_rate, bool capture)
{
    if (!voice_session_owned() || voice_output_open || playback_open)
        return ESP_ERR_INVALID_STATE;
    if (sample_rate != 16000 && sample_rate != 24000) return ESP_ERR_INVALID_ARG;
    if (!playback_lock) playback_lock = xSemaphoreCreateMutex();
    if (!playback_lock) return ESP_ERR_NO_MEM;
    if (!bsp_audio_codec_speaker_init() || (capture && !bsp_audio_codec_microphone_init())) return ESP_ERR_NOT_FOUND;
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    esp_codec_dev_sample_info_t format = {.sample_rate=sample_rate, .channel=2, .bits_per_sample=16};
    esp_err_t ret = ESP_FAIL;
    if (esp_codec_dev_open(speaker_codec, &format) != ESP_CODEC_DEV_OK) goto done;
    voice_output_open = true;
    playback_muted = bsp_audio_session_get_owner() == BSP_AUDIO_OWNER_MIC_TEST;
    playback_volume = -1;
    if (capture) {
        esp_codec_dev_sample_info_t input_format = {
            .sample_rate=sample_rate, .channel=4, .channel_mask=0x0f, .bits_per_sample=16,
        };
        if (esp_codec_dev_open(microphone_codec, &input_format) != ESP_CODEC_DEV_OK) goto fail;
        voice_input_open = true;
        if (esp_codec_dev_set_in_channel_gain(microphone_codec, 0x07, 24.0f) != ESP_CODEC_DEV_OK) goto fail;
    }
    ret = playback_update_volume(true);
    if (ret != ESP_OK) goto fail;
    ESP_LOGI(TAG, "%s audio: %lu Hz, 16-bit, ES8311 stereo, ES7210 TDM slots=MIC1/MIC3(REF)/MIC2/MIC4, capture=%d, volume=%d",
             bsp_audio_owner_name(bsp_audio_session_get_owner()), (unsigned long)sample_rate, capture, bsp_audio_volume_get());
    goto done;
fail:
    bsp_audio_poweramp_enable(false);
    if (voice_input_open) { esp_codec_dev_close(microphone_codec); voice_input_open = false; }
    esp_codec_dev_close(speaker_codec); voice_output_open = false;
    ret = ESP_FAIL;
done:
    xSemaphoreGive(playback_lock);
    return ret;
}
esp_err_t bsp_audio_voice_read(void *pcm, size_t bytes, size_t *read, uint32_t timeout_ms)
{
    if (read) *read = 0;
    if (!pcm || !read || bytes % (BSP_AUDIO_VOICE_CHANNELS * sizeof(int16_t))) return ESP_ERR_INVALID_ARG;
    if (!voice_session_owned() || !voice_input_open) return ESP_ERR_INVALID_STATE;
    /* Bound reads so shutdown can always join the capture task. */
    return i2s_channel_read(i2s_rx_chan, pcm, bytes, read, timeout_ms);
}
esp_err_t bsp_audio_voice_write(void *pcm, size_t bytes, size_t *written, uint32_t timeout_ms)
{
    if (written) *written = 0;
    if (!pcm || !written || bytes % (2 * sizeof(int16_t))) return ESP_ERR_INVALID_ARG;
    if (!voice_session_owned() || !voice_output_open) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    esp_err_t ret = playback_update_volume(false);
    if (ret == ESP_OK) ret = i2s_channel_write(i2s_tx_chan, pcm, bytes, written, timeout_ms);
    xSemaphoreGive(playback_lock);
    return ret;
}
esp_err_t bsp_audio_voice_mute(bool mute)
{
    if (!voice_session_owned() || !voice_output_open) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    playback_muted = mute;
    esp_err_t ret = playback_update_volume(true);
    xSemaphoreGive(playback_lock);
    return ret;
}
esp_err_t bsp_audio_voice_close(void)
{
    if (!voice_session_owned()) return ESP_ERR_INVALID_STATE;
    if (!playback_lock) return ESP_OK;
    /* Caller must stop/join capture, encoder and playback before this call. */
    xSemaphoreTake(playback_lock, portMAX_DELAY);
    bsp_audio_poweramp_enable(false);
    esp_err_t ret = ESP_OK;
    if (voice_input_open) {
        if (esp_codec_dev_close(microphone_codec) == ESP_CODEC_DEV_OK) voice_input_open = false;
        else ret = ESP_FAIL;
    }
    if (voice_output_open) {
        esp_codec_dev_set_out_mute(speaker_codec, true);
        if (esp_codec_dev_close(speaker_codec) == ESP_CODEC_DEV_OK) voice_output_open = false;
        else ret = ESP_FAIL;
    }
    playback_volume = -1; playback_muted = true;
    xSemaphoreGive(playback_lock);
    return ret;
}




static bool bsp_display_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    lv_display_flush_ready((lv_display_t *)user_ctx);
    return false;
}

static void bsp_display_st7305_lut_init(void)
{
    if (st7305_lut_initialized) {
        return;
    }

    for (uint16_t pair_x = 0; pair_x < (BSP_LCD_H_RES / 2); pair_x++) {
        st7305_src_byte_x[pair_x] = pair_x >> 2;
    }

    for (uint16_t block_y = 0; block_y < (BSP_LCD_V_RES / 4); block_y++) {
        for (uint8_t local_y = 0; local_y < 4; local_y++) {
            const uint16_t y = BSP_LCD_V_RES - 1 - ((block_y << 2) + local_y);
            st7305_src_row_offset[block_y][local_y] = y * (BSP_LCD_H_RES / 8);
        }
    }

    for (uint8_t pair_in_byte = 0; pair_in_byte < 4; pair_in_byte++) {
        const uint8_t even_mask = 1 << (7 - (pair_in_byte << 1));
        const uint8_t odd_mask = 1 << (6 - (pair_in_byte << 1));

        for (uint8_t local_y = 0; local_y < 4; local_y++) {
            const uint8_t even_out = 1 << (7 - (local_y << 1));
            const uint8_t odd_out = 1 << (6 - (local_y << 1));

            for (uint16_t src = 0; src < 256; src++) {
                uint8_t out = 0;
                if (src & even_mask) {
                    out |= even_out;
                }
                if (src & odd_mask) {
                    out |= odd_out;
                }
                st7305_pair_lut[pair_in_byte][local_y][src] = out;
            }
        }
    }

    st7305_lut_initialized = true;
}

static void bsp_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    const uint8_t *src = px_map + 8;
    uint8_t *dst = st7305_tx_fb;

    bsp_display_st7305_lut_init();

    for (uint16_t pair_x = 0; pair_x < (BSP_LCD_H_RES / 2); pair_x++) {
        const uint8_t pair_in_byte = pair_x & 0x03;
        const uint16_t src_byte_x = st7305_src_byte_x[pair_x];

        for (uint16_t block_y = 0; block_y < (BSP_LCD_V_RES / 4); block_y++) {
            uint8_t out = 0;

            for (uint8_t local_y = 0; local_y < 4; local_y++) {
                const uint8_t src_byte = src[st7305_src_row_offset[block_y][local_y] + src_byte_x];
                out |= st7305_pair_lut[pair_in_byte][local_y][src_byte];
            }

            *dst++ = out;
        }
    }

    esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, BSP_LCD_H_RES, BSP_LCD_V_RES, st7305_tx_fb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ST7305 flush failed: %s", esp_err_to_name(ret));
        lv_display_flush_ready(disp);
    }
}

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    esp_err_t ret = ESP_OK;
    esp_lcd_panel_io_handle_t rlcd_io = NULL;

    int max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES / 8;
    if (config && config->max_transfer_sz) {
        max_transfer_sz = config->max_transfer_sz;
    }

    ESP_LOGI(TAG, "Initialize SPI bus for ST7305");
    const spi_bus_config_t buscfg = ST7305_PANEL_BUS_SPI_CONFIG(BSP_LCD_PCLK, BSP_LCD_MOSI, max_transfer_sz);
    ESP_GOTO_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), err, TAG, "SPI bus init failed");

    ESP_LOGI(TAG, "Install ST7305 panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = ST7305_PANEL_IO_SPI_CONFIG(BSP_LCD_CS, BSP_LCD_DC, NULL, NULL);
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &rlcd_io),
                      err, TAG, "Panel IO init failed");

    st7305_vendor_config_t vendor_config = {
        .hor_res = BSP_LCD_H_RES,
        .ver_res = BSP_LCD_V_RES,
        .col_start = 0x12,
        .col_end = 0x2A,
        .row_start = 0x00,
        .row_end = 0xC7,
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
        .vendor_config = &vendor_config,
    };

    ESP_LOGI(TAG, "Install ST7305 panel driver");
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7305(rlcd_io, &panel_config, &panel_handle),
                      err, TAG, "ST7305 driver init failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(panel_handle), err, TAG, "Panel reset failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(panel_handle), err, TAG, "Panel init failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), err, TAG, "Turn on display failed");

    io_handle = rlcd_io;
    if (ret_panel) {
        *ret_panel = panel_handle;
    }
    if (ret_io) {
        *ret_io = io_handle;
    }

    return ESP_OK;

err:
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
        panel_handle = NULL;
    }
    if (rlcd_io) {
        esp_lcd_panel_io_del(rlcd_io);
    }
    spi_bus_free(BSP_LCD_SPI_NUM);
    return ret;
}

static void bsp_touch_process_coordinates(esp_lcd_touch_handle_t tp,
                                          uint16_t *x,
                                          uint16_t *y,
                                          uint16_t *strength,
                                          uint8_t *point_num,
                                          uint8_t max_point_num)
{
    (void)tp;
    (void)strength;

    uint8_t points = *point_num;
    if (points > max_point_num) {
        points = max_point_num;
    }

    for (uint8_t i = 0; i < points; i++) {
        int32_t tx = x[i];
        int32_t ty = y[i];

        if (tx < 140 && ty > 200) {
            int32_t dx = 140 - tx;
            int32_t dy = ty - 200;
            ty += (dx * dy * 60) / (140 * 95);
        }

        if (tx < 0) {
            tx = 0;
        } else if (tx >= BSP_LCD_H_RES) {
            tx = BSP_LCD_H_RES - 1;
        }

        if (ty < 0) {
            ty = 0;
        } else if (ty >= BSP_LCD_V_RES) {
            ty = BSP_LCD_V_RES - 1;
        }

        x[i] = tx;
        y[i] = ty;
    }
}

esp_err_t bsp_touch_new(const bsp_touch_config_t *config, esp_lcd_touch_handle_t *ret_touch)
{
    (void)config;
    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_LCD_TOUCH_RST,
        .int_gpio_num = BSP_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .process_coordinates = bsp_touch_process_coordinates,
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    tp_io_config.scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, ret_touch);
}

static lv_display_t *bsp_display_lcd_init(void)
{
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    esp_lcd_panel_handle_t lcd_panel = NULL;
    bsp_display_config_t disp_config = {
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES / 8,
    };
    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new(&disp_config, &lcd_panel, &lcd_io));

    lv_display_t *display = lv_display_create(BSP_LCD_H_RES, BSP_LCD_V_RES);
    BSP_NULL_CHECK(display, NULL);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_I1);
    lv_display_set_buffers(display, lvgl_i1_fb, NULL, sizeof(lvgl_i1_fb), LV_DISPLAY_RENDER_MODE_FULL);
    bsp_display_st7305_lut_init();
    lv_display_set_flush_cb(display, bsp_display_flush);

    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = bsp_display_flush_ready,
    };
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_io_register_event_callbacks(lcd_io, &cbs, display));

    return display;
}

static lv_indev_t *bsp_display_indev_init(lv_display_t *disp)
{
    BSP_ERROR_CHECK_RETURN_NULL(bsp_touch_new(NULL, &tp));
    assert(tp);

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

lv_display_t *bsp_display_start(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
        },
    };

    cfg.lvgl_port_cfg.task_priority = 5;
    cfg.lvgl_port_cfg.task_stack = 12288;
    cfg.lvgl_port_cfg.task_affinity = 1;
    cfg.lvgl_port_cfg.task_max_sleep_ms = 20;
    cfg.lvgl_port_cfg.timer_period_ms = 2;

    return bsp_display_start_with_config(&cfg);
}

lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg)
{
    assert(cfg != NULL);
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&cfg->lvgl_port_cfg));
    BSP_NULL_CHECK(disp = bsp_display_lcd_init(), NULL);
    BSP_NULL_CHECK(disp_indev = bsp_display_indev_init(disp), NULL);
    return disp;
}

lv_indev_t *bsp_display_get_input_dev(void)
{
    return disp_indev;
}


bool bsp_display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}
