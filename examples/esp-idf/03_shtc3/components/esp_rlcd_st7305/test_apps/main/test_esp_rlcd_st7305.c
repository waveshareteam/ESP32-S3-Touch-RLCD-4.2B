/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rlcd_st7305.h"

/* RLCD Dimensions */
#define RLCD_H_RES          (400)
#define RLCD_V_RES          (300)

/* RLCD SPI Configuration */
#define RLCD_SPI_NUM        (SPI3_HOST)
#define RLCD_PIXEL_CLK_HZ   (10 * 1000 * 1000)
#define RLCD_CMD_BITS       (8)
#define RLCD_PARAM_BITS     (8)
#define RLCD_BITS_PER_PIXEL (1)

/* ESP32-S3-Touch-RLCD-4.2 factory demo pin definitions */
#define RLCD_GPIO_SCLK      (GPIO_NUM_11)
#define RLCD_GPIO_MOSI      (GPIO_NUM_12)
#define RLCD_GPIO_RST       (GPIO_NUM_41)
#define RLCD_GPIO_DC        (GPIO_NUM_5)
#define RLCD_GPIO_CS        (GPIO_NUM_40)

static const char *TAG = "RLCD_DRIVER";
static esp_lcd_panel_io_handle_t rlcd_io = NULL;
static esp_lcd_panel_handle_t rlcd_panel = NULL;
static st7305_frame_buffer_t rlcd_fb;

static esp_err_t rlcd_flush(void)
{
    return esp_rlcd_st7305_frame_buffer_flush(rlcd_panel, &rlcd_fb);
}

static void rlcd_fill_screen(uint8_t color)
{
    esp_rlcd_st7305_frame_buffer_clear(&rlcd_fb, color);
    ESP_ERROR_CHECK(rlcd_flush());
}


static void rlcd_draw_checkerboard(uint8_t block_size)
{
    esp_rlcd_st7305_frame_buffer_clear(&rlcd_fb, ST7305_COLOR_WHITE);

    for (uint16_t y = 0; y < RLCD_V_RES; y++) {
        for (uint16_t x = 0; x < RLCD_H_RES; x++) {
            uint8_t color = (((x / block_size) + (y / block_size)) & 0x01) ? ST7305_COLOR_BLACK : ST7305_COLOR_WHITE;
            ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_set_pixel(&rlcd_fb, x, y, color));
        }
    }

    ESP_ERROR_CHECK(rlcd_flush());
}

static void rlcd_draw_diagonal_lines(void)
{
    esp_rlcd_st7305_frame_buffer_clear(&rlcd_fb, ST7305_COLOR_WHITE);

    ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_draw_line(&rlcd_fb, 0, 0, RLCD_H_RES - 1, RLCD_V_RES - 1, ST7305_COLOR_BLACK));
    ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_draw_line(&rlcd_fb, RLCD_H_RES - 1, 0, 0, RLCD_V_RES - 1, ST7305_COLOR_BLACK));
    ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_draw_rect(&rlcd_fb, 8, 8, RLCD_H_RES - 16, RLCD_V_RES - 16, ST7305_COLOR_BLACK));
    ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_fill_rect(&rlcd_fb, 32, 32, 64, 40, ST7305_COLOR_BLACK));
    ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_fill_rect(&rlcd_fb, RLCD_H_RES - 96, RLCD_V_RES - 72, 64, 40, ST7305_COLOR_BLACK));

    ESP_ERROR_CHECK(rlcd_flush());
}

static esp_err_t rlcd_spi_bus_init(void)
{
    const spi_bus_config_t buscfg = ST7305_PANEL_BUS_SPI_CONFIG(
        RLCD_GPIO_SCLK,
        RLCD_GPIO_MOSI,
        RLCD_H_RES * RLCD_V_RES / 8
    );

    return spi_bus_initialize(RLCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO);
}

static esp_err_t rlcd_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initialize SPI bus for ST7305");
    ESP_GOTO_ON_ERROR(rlcd_spi_bus_init(), err, TAG, "SPI bus init failed");

    ESP_LOGI(TAG, "Install LCD panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = RLCD_GPIO_DC,
        .cs_gpio_num = RLCD_GPIO_CS,
        .pclk_hz = RLCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = RLCD_CMD_BITS,
        .lcd_param_bits = RLCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)RLCD_SPI_NUM, &io_config, &rlcd_io),
                      err, TAG, "Panel IO init failed");

    st7305_vendor_config_t vendor_config = {
        .hor_res = RLCD_H_RES,
        .ver_res = RLCD_V_RES,
        .col_start = 0x12,
        .col_end = 0x2A,
        .row_start = 0x00,
        .row_end = 0xC7,
    };

    ESP_LOGI(TAG, "Install ST7305 panel driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = RLCD_GPIO_RST,
        .bits_per_pixel = RLCD_BITS_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7305(rlcd_io, &panel_config, &rlcd_panel),
                      err, TAG, "ST7305 driver init failed");

    ESP_LOGI(TAG, "Reset and init ST7305 panel");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(rlcd_panel), err, TAG, "Panel reset failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(rlcd_panel), err, TAG, "Panel init failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_disp_on_off(rlcd_panel, true), err, TAG, "Turn on display failed");

    ESP_LOGI(TAG, "Create 1bpp framebuffer and lookup table");
    ESP_GOTO_ON_ERROR(esp_rlcd_st7305_frame_buffer_init(&rlcd_fb,
                                                        RLCD_H_RES,
                                                        RLCD_V_RES,
                                                        ST7305_PIXEL_MAPPING_LANDSCAPE,
                                                        MALLOC_CAP_8BIT),
                      err, TAG, "Frame buffer init failed");

    rlcd_fill_screen(ST7305_COLOR_WHITE);

    ESP_LOGI(TAG, "ST7305 RLCD init success");
    return ESP_OK;

err:
    esp_rlcd_st7305_frame_buffer_deinit(&rlcd_fb);
    if (rlcd_panel) {
        esp_lcd_panel_del(rlcd_panel);
        rlcd_panel = NULL;
    }
    if (rlcd_io) {
        esp_lcd_panel_io_del(rlcd_io);
        rlcd_io = NULL;
    }
    spi_bus_free(RLCD_SPI_NUM);
    ESP_LOGE(TAG, "ST7305 RLCD init failed (err: %s)", esp_err_to_name(ret));
    return ret;
}

void app_main(void)
{
    ESP_ERROR_CHECK(rlcd_init());

    while (1) {
        ESP_LOGI(TAG, "Fill white");
        rlcd_fill_screen(ST7305_COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Fill black");
        rlcd_fill_screen(ST7305_COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Draw checkerboard");
        rlcd_draw_checkerboard(20);
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "Draw diagonal lines");
        rlcd_draw_diagonal_lines();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
