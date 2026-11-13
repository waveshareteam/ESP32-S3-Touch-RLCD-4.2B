idf_component_register(
    SRCS "Settings.cpp" "ui/settings_ui.c" "service/settings_service.c"
         "assets/settings_icons.c"
    INCLUDE_DIRS "." "ui" "service" "assets"
    REQUIRES brookesia_core lvgl esp32_s3_touch_rlcd_4_2b network_service rlcd_ui
    PRIV_REQUIRES esp_app_format esp_system esp_hw_support spi_flash esp_psram
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
