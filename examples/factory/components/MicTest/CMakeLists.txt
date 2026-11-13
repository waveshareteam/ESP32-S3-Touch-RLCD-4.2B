idf_component_register(
    SRCS "MicTest.cpp" "assets/img_app_mic_test.c"
    INCLUDE_DIRS "."
    REQUIRES brookesia_core lvgl esp32_s3_touch_rlcd_4_2b rlcd_ui
    PRIV_REQUIRES esp_timer esp_psram
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
