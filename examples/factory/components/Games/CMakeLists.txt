idf_component_register(
    SRCS
        "Games.cpp"
        "game_init/game_init.c"
        "generated/setup_scr_Game2048.c"
        "generated/events_init.c"
        "custom/custom.c"
        "custom/game_2048.c"
        "assets/image_2048_mono.c"
        "generated/guider_fonts/lv_font_Alatsi_Regular_16.c"
        "generated/guider_fonts/lv_font_Alatsi_Regular_40.c"
        "generated/guider_fonts/lv_font_Alatsi_Regular_18.c"
        "generated/guider_fonts/lv_font_Alatsi_Regular_33.c"
        "generated/guider_fonts/lv_font_Alatsi_Regular_21.c"
        "generated/guider_fonts/lv_font_Acme_Regular_16.c"
    INCLUDE_DIRS "." "game_init" "generated" "custom"
    REQUIRES lvgl brookesia_core esp_hw_support rlcd_ui
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
