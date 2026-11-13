idf_component_register(
    SRCS "esp32_s3_touch_rlcd_4_2b.c" "bsp_audio_volume.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_i2s
        esp_driver_sdmmc
        esp_lcd
        esp_lcd_touch
        esp_lcd_touch_cst816s
        esp_rlcd_st7305
        esp_lvgl_port
        lvgl
    PRIV_REQUIRES esp_timer esp_adc spiffs fatfs esp_psram waveshare__pcf85063a nvs_flash
)
