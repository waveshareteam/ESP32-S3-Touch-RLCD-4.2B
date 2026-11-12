idf_component_register(
    SRCS "DrawingBoard.cpp" "assets/image_draw_mono.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl brookesia_core
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
