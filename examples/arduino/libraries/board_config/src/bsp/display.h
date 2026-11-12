#include "rlcd_arduino_config.h"
#pragma once

#include "esp_err.h"
#include "esp_lcd_types.h"

#define ESP_LCD_COLOR_FORMAT_MONO    (0)

#define BSP_LCD_COLOR_FORMAT         (ESP_LCD_COLOR_FORMAT_MONO)
#define BSP_LCD_BITS_PER_PIXEL       (1)
#define BSP_LCD_H_RES                (400)
#define BSP_LCD_V_RES                (300)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int max_transfer_sz;    /*!< Maximum transfer size, in bytes. */
} bsp_display_config_t;

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io);


#ifdef __cplusplus
}
#endif
