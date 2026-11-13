idf_component_register(
    SRCS "main.cpp"
    PRIV_REQUIRES
        esp32_s3_touch_rlcd_4_2b
        brookesia_core
        DrawingBoard
        Games
        Weather
        Photos
        Sensor
        Settings
        Music
        XiaozhiApp
        ButtonTest
        DisplayTest
        MicTest
        network_service
        rlcd_desktop
        nvs_flash
    INCLUDE_DIRS "."
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
