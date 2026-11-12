idf_component_register(
    SRCS "Music.cpp" "ui/music_ui.c"
         "service/music_service.c" "assets/music_assets.c"
    INCLUDE_DIRS "." "ui" "service" "assets"
    REQUIRES brookesia_core lvgl esp32_s3_touch_rlcd_4_2 rlcd_ui
    PRIV_REQUIRES esp_audio_simple_player espressif__gmf_core fatfs
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
