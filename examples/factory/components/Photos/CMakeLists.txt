idf_component_register(
    SRCS "Photos.cpp" "photo_init/photo_init.c"
         "service/photo_service.c" "service/photo_result.c" "assets/image_photos_mono.c"
    INCLUDE_DIRS "." "photo_init" "service"
    REQUIRES brookesia_core lvgl esp32_s3_touch_rlcd_4_2b esp_new_jpeg rlcd_ui
    PRIV_REQUIRES esp_timer
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
