#include "rlcd_arduino_config.h"
#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/sdmmc_host.h"
#include "bsp/config.h"
#include "bsp/display.h"
#include "esp_codec_dev.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

/**************************************************************************************************
 *  BSP Capabilities
 **************************************************************************************************/

#define BSP_CAPS_DISPLAY        1
#define BSP_CAPS_TOUCH          1
#define BSP_CAPS_BUTTONS        1
#define BSP_CAPS_AUDIO          1
#define BSP_CAPS_AUDIO_SPEAKER  1
#define BSP_CAPS_AUDIO_MIC      1
#define BSP_CAPS_SDCARD         1
#define BSP_CAPS_IMU            0
#define BSP_CAPS_RTC            1
#define BSP_CAPS_ENVIRONMENT    1
#define BSP_CAPS_BATTERY        1

/**************************************************************************************************
 * ESP32-S3-Touch-RLCD-4.2B pinout
 **************************************************************************************************/

/* User key: active low by default (switch to GND). */
#define BSP_USER_BUTTON_IO           (GPIO_NUM_18)
#define BSP_USER_BUTTON_ACTIVE_LEVEL 0
#define BSP_BOOT_BUTTON_IO           (GPIO_NUM_0)
#define BSP_BOOT_BUTTON_ACTIVE_LEVEL 0

/* VBAT divided by 3: R21=200k, R23=100k; ADC1 channel 3. */
#define BSP_BATTERY_ADC_IO      (GPIO_NUM_4)

/* I2C */
#define BSP_I2C_SCL             (GPIO_NUM_14)
#define BSP_I2C_SDA             (GPIO_NUM_13)

/* I2S audio */
#define BSP_I2S_SCLK            (GPIO_NUM_9)
#define BSP_I2S_MCLK            (GPIO_NUM_16)
#define BSP_I2S_LCLK            (GPIO_NUM_45)
#define BSP_I2S_DOUT            (GPIO_NUM_8)
#define BSP_I2S_DSIN            (GPIO_NUM_10)
#define BSP_POWER_AMP_IO        (GPIO_NUM_46)

/* RLCD ST7305 */
#define BSP_LCD_CS              (GPIO_NUM_40)
#define BSP_LCD_DC              (GPIO_NUM_5)
#define BSP_LCD_MOSI            (GPIO_NUM_12)
#define BSP_LCD_PCLK            (GPIO_NUM_11)
#define BSP_LCD_RST             (GPIO_NUM_41)
// #define BSP_LCD_RST             (GPIO_NUM_NC)
#define BSP_LCD_BACKLIGHT       (GPIO_NUM_NC)

/* CST816 touch on BSP_I2C_SDA/SCL, polled without reset/interrupt GPIOs. */
#define BSP_LCD_TOUCH_RST       (GPIO_NUM_NC)
#define BSP_LCD_TOUCH_INT       (GPIO_NUM_NC)

/* uSD card, SDMMC 1-bit. Schematic MOSI/MISO are CMD/D0 in this mode.
 * R7 is NC: GPIO17 is not connected and no GPIO-controlled CS is used. */
#define BSP_SD_D0               (GPIO_NUM_39)
#define BSP_SD_CMD              (GPIO_NUM_21)
#define BSP_SD_CLK              (GPIO_NUM_38)

#define BSP_LCD_SPI_CLK_HZ      (40 * 1000 * 1000)
#define BSP_LCD_SPI_NUM         (SPI3_HOST)
#define LVGL_BUFFER_HEIGHT      (CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT)

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_I2C_NUM             CONFIG_BSP_I2C_NUM

esp_err_t bsp_i2c_init(void);
esp_err_t bsp_i2c_deinit(void);

/* Call init once at startup. Active-low, internally pulled up; 30 ms debounce.
 * Count increases once per press, with no long-press repeats. Safe to read from UI. */
esp_err_t bsp_user_button_init(void);
uint32_t bsp_user_button_get_press_count(void);

typedef enum {
    BSP_BUTTON_ID_BOOT,
    BSP_BUTTON_ID_USER,
} bsp_button_id_t;

/* Button Test reads raw levels; its LVGL timer debounces complete press/release cycles.
 * The existing GPIO18 desktop press counter remains independent. */
esp_err_t bsp_button_test_init(void);
esp_err_t bsp_button_get_pressed(bsp_button_id_t id, bool *pressed);
i2c_master_bus_handle_t bsp_i2c_get_handle(void);

/**************************************************************************************************
 * Battery voltage and estimated percentage (no charging-state detection)
 **************************************************************************************************/
typedef struct {
    int voltage_mv;  /* Averaged and smoothed battery-side millivolts. */
    int percent;     /* Voltage estimate 0..100; -1 when unavailable. */
} bsp_battery_info_t;

/* Call init once at startup, before starting APPs. Samples every 500 ms;
 * first result after 10 good samples (~5 s), then 1/4 exponential smoothing.
 * get_info only copies a cache; safe for UI, no ADC access or blocking waits.
 * Returns INVALID_STATE before first result, TIMEOUT if >15 s old.
 * No reliable battery-presence or charging detection is provided. */
esp_err_t bsp_battery_init(void);
esp_err_t bsp_battery_get_info(bsp_battery_info_t *info);

/**************************************************************************************************
 * RTC and temperature/humidity (shared BSP I2C bus)
 **************************************************************************************************/
typedef struct {
    int year, month, day, weekday, hour, minute, second;
} bsp_sensor_datetime_t;

/* Serialized hardware access: call from a worker without holding the GUI lock.
 * Errors are returned to the caller; missing devices do not abort the firmware.
 * Same as C5: each pcf_init() writes the factory-test Set_Time. */
esp_err_t pcf_init(void);
esp_err_t shtc3_init(void);
esp_err_t bsp_sensor_read_environment(float *temperature, float *humidity);
/* PCF85063A native date API: years 1970..2069, weekday 0..6. */
esp_err_t bsp_sensor_read_datetime(bsp_sensor_datetime_t *datetime);
esp_err_t bsp_sensor_set_datetime(const bsp_sensor_datetime_t *datetime);

esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config);
esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void);
esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void);
esp_err_t bsp_audio_poweramp_enable(bool enable);

/* Call init once after nvs_flash_init, before starting APPs. get/set are memory
 * only and safe from the GUI task; playback applies changes at the next PCM
 * block or resume. Save the released slider value from one background worker. */
#define BSP_AUDIO_VOLUME_DEFAULT 55
esp_err_t bsp_audio_volume_init(void);
int bsp_audio_volume_get(void);
esp_err_t bsp_audio_volume_set(int volume);
esp_err_t bsp_audio_volume_save(int volume);

/* Music PCM output. Single playback worker owns open/mute/write/close; UI never
 * accesses the codec. Stop the decoder before close. Hardware stays in BSP. */
#define BSP_AUDIO_PLAYBACK_RATE 24000
#define BSP_AUDIO_PLAYBACK_CHANNELS 1
#define BSP_AUDIO_PLAYBACK_BITS 16
esp_err_t bsp_audio_playback_open(void);
esp_err_t bsp_audio_playback_write(void *pcm, size_t bytes);
/* After the producer pauses/finishes, let the short I2S DMA tail play out. */
void bsp_audio_playback_drain(void);
esp_err_t bsp_audio_playback_mute(bool mute);
esp_err_t bsp_audio_playback_close(void);

/* Exclusive shared I2S session. Music claims automatically in playback_open;
 * Xiaozhi and Mic Test claim before opening codecs and release after all I/O tasks join. */
typedef enum {
    BSP_AUDIO_OWNER_NONE, BSP_AUDIO_OWNER_MUSIC, BSP_AUDIO_OWNER_XIAOZHI, BSP_AUDIO_OWNER_MIC_TEST
} bsp_audio_owner_t;
esp_err_t bsp_audio_session_acquire(bsp_audio_owner_t owner);
esp_err_t bsp_audio_session_release(bsp_audio_owner_t owner);
bsp_audio_owner_t bsp_audio_session_get_owner(void);
const char *bsp_audio_owner_name(bsp_audio_owner_t owner);

/* RLCD ES7210 TDM slots: MIC1, MIC3 echo reference, MIC2, unused MIC4.
 * Voice output is stereo (duplicated mono); Music retains its mono API.
 * Xiaozhi or Mic Test must own the session. Mic Test opens with output muted. */
#define BSP_AUDIO_VOICE_CHANNELS 4
esp_err_t bsp_audio_voice_open(uint32_t sample_rate, bool capture);
esp_err_t bsp_audio_voice_read(void *pcm, size_t bytes, size_t *read, uint32_t timeout_ms);
esp_err_t bsp_audio_voice_write(void *pcm, size_t bytes, size_t *written, uint32_t timeout_ms);
esp_err_t bsp_audio_voice_mute(bool mute);
esp_err_t bsp_audio_voice_close(void);

#define BSP_SPIFFS_MOUNT_POINT  CONFIG_BSP_SPIFFS_MOUNT_POINT

esp_err_t bsp_spiffs_mount(void);
esp_err_t bsp_spiffs_unmount(void);

#define BSP_SD_MOUNT_POINT      CONFIG_BSP_SD_MOUNT_POINT
extern sdmmc_card_t *bsp_sdcard;

esp_err_t bsp_sdcard_mount(void);
esp_err_t bsp_sdcard_unmount(void);
/* Long-lived SD readers hold a lease; remount/unmount is refused while leased.
 * Acquire mounts if necessary; release does not unmount the shared card. */
esp_err_t bsp_sdcard_acquire(void);
void bsp_sdcard_release(void);
typedef struct {
    bool mounted;
    uint64_t capacity_bytes;
    uint64_t total_bytes;
    uint64_t free_bytes;
} bsp_sdcard_info_t;
/* Query the mounted FAT volume without scanning or decoding any files.
 * Call from a worker: free-space lookup may read the card. */
esp_err_t bsp_sdcard_get_info(bsp_sdcard_info_t *info);

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
#define BSP_LCD_DRAW_BUFF_SIZE  (BSP_LCD_H_RES * LVGL_BUFFER_HEIGHT)
#define BSP_LCD_DRAW_BUFF_DOUBLE (0)

typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg;  /*!< LVGL port configuration */
    uint32_t        buffer_size;    /*!< Size of the buffer for the screen in pixels */
    uint32_t        trans_size;
    bool            double_buffer;  /*!< True, if should be allocated two buffers */
    struct {
        unsigned int buff_dma: 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram: 1; /*!< Allocated LVGL buffer will be in PSRAM */
    } flags;
} bsp_display_cfg_t;

lv_display_t *bsp_display_start(void);
lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);
lv_indev_t *bsp_display_get_input_dev(void);
bool bsp_display_lock(uint32_t timeout_ms);
void bsp_display_unlock(void);
#endif

#ifdef __cplusplus
}
#endif
