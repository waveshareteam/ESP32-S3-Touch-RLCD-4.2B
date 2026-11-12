idf_component_register(
    SRCS "rlcd_desktop.c" "assets/desktop_faces.c" "assets/rlcd_font_status_16.c"
    INCLUDE_DIRS "." "assets"
    REQUIRES lvgl brookesia_core esp32_s3_touch_rlcd_4_2 network_service
)

