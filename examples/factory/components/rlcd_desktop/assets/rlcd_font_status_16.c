/*
 * Compact 8x16, 1-bit status font: digits, colon, dash and percent.
 * Derived from LVGL's src/font/lv_font_unscii_8.c (UNSCII).
 * Each source bitmap row is repeated twice offline; widths are unchanged.
 * No runtime scaling, gray pixels or smoothing. The glyph bitmaps use 128 bytes.
 * Keep the Montserrat font for LV_SYMBOL_* icons.
 */
#include "lvgl.h"

static const uint8_t glyph_bitmap[] = {
    /* % */
    0xc7, 0x8f, 0x36, 0x61, 0x83, 0x0c, 0x18, 0x66, 0xcf, 0x1e, 0x30,
    /* - */
    0xff, 0xf0,
    /* 0 */
    0x79, 0xec, 0xf3, 0xdf, 0x7e, 0xfb, 0xcf, 0x3c, 0xf3, 0x79, 0xe0,
    /* 1 */
    0x30, 0xc7, 0x1c, 0x30, 0xc3, 0x0c, 0x30, 0xc3, 0x0c, 0xff, 0xf0,
    /* 2 */
    0x79, 0xec, 0xf3, 0x18, 0x63, 0x0c, 0x61, 0x8c, 0x30, 0xff, 0xf0,
    /* 3 */
    0x79, 0xec, 0xf3, 0x0c, 0x33, 0x8e, 0x0c, 0x3c, 0xf3, 0x79, 0xe0,
    /* 4 */
    0x1c, 0x38, 0xf1, 0xe6, 0xcd, 0xb3, 0x66, 0xff, 0xfc, 0x30, 0x60, 0xc1, 0x80,
    /* 5 */
    0xff, 0xfc, 0x30, 0xfb, 0xe0, 0xc3, 0x0c, 0x3c, 0xf3, 0x79, 0xe0,
    /* 6 */
    0x38, 0xe6, 0x18, 0xc3, 0x0f, 0xbe, 0xcf, 0x3c, 0xf3, 0x79, 0xe0,
    /* 7 */
    0xff, 0xf0, 0xc3, 0x0c, 0x31, 0x86, 0x30, 0xc3, 0x0c, 0x30, 0xc0,
    /* 8 */
    0x79, 0xec, 0xf3, 0xcf, 0x37, 0x9e, 0xcf, 0x3c, 0xf3, 0x79, 0xe0,
    /* 9 */
    0x79, 0xec, 0xf3, 0xcf, 0x37, 0xdf, 0x0c, 0x31, 0x86, 0x71, 0xc0,
    /* : */
    0xff, 0x00, 0xff,
};

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {0}, /* Missing glyph */
    {.bitmap_index = 0, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 11, .adv_w = 128, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 13, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 24, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 35, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 46, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 57, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 70, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 81, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 92, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 103, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 114, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 125, .adv_w = 128, .box_w = 2, .box_h = 12, .ofs_x = 3, .ofs_y = 2},
};

static const uint16_t unicode_list[] = {
    0, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21
};

static const lv_font_fmt_txt_cmap_t cmaps[] = {{
    .range_start = 37,
    .range_length = 22,
    .glyph_id_start = 1,
    .unicode_list = unicode_list,
    .list_length = 13,
    .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
}};

static const lv_font_fmt_txt_dsc_t font_dsc = {
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .cmap_num = 1,
    .bpp = 1,
    .bitmap_format = 0,
};

const lv_font_t rlcd_font_status_16 = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 18,
    .base_line = 0,
    .subpx = LV_FONT_SUBPX_NONE,
    .dsc = &font_dsc,
};
