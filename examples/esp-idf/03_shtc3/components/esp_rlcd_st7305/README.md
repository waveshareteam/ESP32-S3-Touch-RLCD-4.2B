# ESP LCD ST7305 RLCD

[English](README.md) | [简体中文](README_ZH.md)

ESP-IDF `esp_lcd` panel driver for ST7305 reflective LCD panels. The default command table and 1bpp framebuffer mapping are ported from the factory `DisplayPort` code in this project.

## Add component

This component can be placed in your project's `components/esp_rlcd_st7305` directory.

## SPI panel setup

```c
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_rlcd_st7305.h"

#define RLCD_H_RES 400
#define RLCD_V_RES 300

spi_bus_config_t bus_config = ST7305_PANEL_BUS_SPI_CONFIG(
    RLCD_SCK_PIN,
    RLCD_MOSI_PIN,
    RLCD_H_RES * RLCD_V_RES / 8
);
ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO));

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_io_spi_config_t io_config = ST7305_PANEL_IO_SPI_CONFIG(
    RLCD_CS_PIN,
    RLCD_DC_PIN,
    NULL,
    NULL
);
ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &io_handle));

st7305_vendor_config_t vendor_config = {
    .hor_res = RLCD_H_RES,
    .ver_res = RLCD_V_RES,
    .col_start = 0x12,
    .col_end = 0x2A,
    .row_start = 0x00,
    .row_end = 0xC7,
};

esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = RLCD_RST_PIN,
    .bits_per_pixel = 1,
    .vendor_config = &vendor_config,
};

esp_lcd_panel_handle_t panel_handle = NULL;
ESP_ERROR_CHECK(esp_lcd_new_panel_st7305(io_handle, &panel_config, &panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
```

## 1bpp framebuffer and LUT

```c
st7305_frame_buffer_t fb;
ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_init(
    &fb,
    RLCD_H_RES,
    RLCD_V_RES,
    ST7305_PIXEL_MAPPING_LANDSCAPE,
    MALLOC_CAP_SPIRAM
));

esp_rlcd_st7305_frame_buffer_clear(&fb, ST7305_COLOR_WHITE);
ESP_ERROR_CHECK(esp_rlcd_st7305_frame_buffer_set_pixel(&fb, 10, 10, ST7305_COLOR_BLACK));

ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, RLCD_H_RES, RLCD_V_RES, fb.buffer));
```

`esp_lcd_panel_draw_bitmap()` expects a full-frame packed 1bpp ST7305 buffer. Pixel mapping is handled by the framebuffer/LVGL conversion layer, matching the original factory program.
