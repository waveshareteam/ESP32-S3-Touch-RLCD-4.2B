idf_component_register(
    SRCS "wifi_manager.c" "wifi_credentials.c" "portal_dns.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_server esp_timer lwip
    EMBED_FILES "root.html"
)
