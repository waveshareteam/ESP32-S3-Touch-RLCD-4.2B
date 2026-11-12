idf_component_register(SRCS "main.c" "clock_font.c"
    INCLUDE_DIRS "." REQUIRES example_support esp_wifi esp_event esp_netif lwip)
