idf_component_register(
    SRCS "Sensor.cpp" "sensor_init/sensor_init.c"
         "service/sensor_service.c" "assets/image_sensor_mono.c"
    INCLUDE_DIRS "." "sensor_init" "service"
    REQUIRES brookesia_core lvgl esp32_s3_touch_rlcd_4_2b rlcd_ui
    PRIV_REQUIRES esp_timer
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
