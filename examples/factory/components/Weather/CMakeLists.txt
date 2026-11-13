idf_component_register(
    SRCS "Weather.cpp" "weather_init/weather_init.c" "ui/weather_ui.c"
         "custom/weather_model.c" "service/weather_service.c" "service/weather_body.c" "assets/weather_icons.c"
         "assets/image_weather_mono.c"
    INCLUDE_DIRS "." "weather_init" "ui" "custom" "service" "assets"
    REQUIRES lvgl brookesia_core network_service esp_http_client esp_timer rlcd_ui
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
