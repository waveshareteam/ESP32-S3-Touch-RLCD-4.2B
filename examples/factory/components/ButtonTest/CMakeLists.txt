idf_component_register(
    SRCS "ButtonTest.cpp" "assets/img_app_button_test.c"
    INCLUDE_DIRS "."
    REQUIRES brookesia_core lvgl esp32_s3_touch_rlcd_4_2b rlcd_ui
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
