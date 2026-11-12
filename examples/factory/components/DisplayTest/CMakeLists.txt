idf_component_register(
    SRCS "DisplayTest.cpp" "assets/img_app_display_test.c"
    INCLUDE_DIRS "."
    REQUIRES brookesia_core lvgl
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
