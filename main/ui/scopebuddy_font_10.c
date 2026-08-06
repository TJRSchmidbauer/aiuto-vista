/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Font data for the Aiuto-Vista fork of ScopeBuddy
 * (https://github.com/johannesboernsen/ScopeBuddy). Montserrat is
 * licensed under the SIL Open Font License 1.1 (see LICENSES/OFL-1.1.txt).
 * Opts: --font managed_components/lvgl__lvgl/scripts/built_in_font/Montserrat-Medium.ttf --size 10 --range 0x20-0x7E,0xA0-0xFF --no-compress --no-prefilter --bpp 1 --format lvgl --lv-include lvgl.h --output main/ui/scopebuddy_font_10.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef SCOPEBUDDY_FONT_10
#define SCOPEBUDDY_FONT_10 1
#endif

#if SCOPEBUDDY_FONT_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xf2,

    /* U+0022 "\"" */
    0xb4,

    /* U+0023 "#" */
    0x24, 0x51, 0xf9, 0x4f, 0xc9, 0x12, 0x0,

    /* U+0024 "$" */
    0x23, 0xa9, 0x47, 0x14, 0xbe, 0x20,

    /* U+0025 "%" */
    0xe4, 0xa4, 0xa8, 0xf6, 0x29, 0x29, 0x6,

    /* U+0026 "&" */
    0x78, 0x91, 0x63, 0x88, 0xd0, 0x9e, 0x80,

    /* U+0027 "'" */
    0xc0,

    /* U+0028 "(" */
    0x6a, 0xaa, 0x40,

    /* U+0029 ")" */
    0xa5, 0x55, 0x80,

    /* U+002A "*" */
    0x23, 0x9c,

    /* U+002B "+" */
    0x27, 0xc8, 0x40,

    /* U+002C "," */
    0xe0,

    /* U+002D "-" */
    0xc0,

    /* U+002E "." */
    0xc0,

    /* U+002F "/" */
    0x12, 0x22, 0x44, 0x48, 0x80,

    /* U+0030 "0" */
    0x7b, 0x38, 0x61, 0x87, 0x37, 0x80,

    /* U+0031 "1" */
    0xe4, 0x92, 0x48,

    /* U+0032 "2" */
    0x70, 0x42, 0x22, 0x23, 0xe0,

    /* U+0033 "3" */
    0xf8, 0x88, 0xe0, 0x87, 0xc0,

    /* U+0034 "4" */
    0x10, 0x86, 0x12, 0xfc, 0x20, 0x80,

    /* U+0035 "5" */
    0x7c, 0x21, 0xe0, 0x87, 0xc0,

    /* U+0036 "6" */
    0x7e, 0x21, 0xe8, 0xc5, 0xc0,

    /* U+0037 "7" */
    0xfe, 0x20, 0x84, 0x10, 0x82, 0x0,

    /* U+0038 "8" */
    0x72, 0x28, 0x9c, 0x8e, 0x17, 0x80,

    /* U+0039 "9" */
    0x74, 0x62, 0xf0, 0x8f, 0xc0,

    /* U+003A ":" */
    0xc0, 0xc0,

    /* U+003B ";" */
    0xc0, 0xe0,

    /* U+003C "<" */
    0x36, 0x18, 0x30,

    /* U+003D "=" */
    0xf0, 0xf0,

    /* U+003E ">" */
    0xc1, 0xd9, 0x0,

    /* U+003F "?" */
    0x74, 0x42, 0x22, 0x0, 0x80,

    /* U+0040 "@" */
    0x1f, 0x18, 0x6d, 0xee, 0x89, 0xa2, 0x68, 0x9d,
    0xf9, 0x80, 0x3c, 0x0,

    /* U+0041 "A" */
    0x10, 0x50, 0xa2, 0x27, 0xc8, 0x60, 0x80,

    /* U+0042 "B" */
    0xfa, 0x18, 0x7e, 0x86, 0x1f, 0x80,

    /* U+0043 "C" */
    0x3d, 0x18, 0x20, 0x81, 0x13, 0xc0,

    /* U+0044 "D" */
    0xf9, 0xa, 0xc, 0x18, 0x30, 0xbe, 0x0,

    /* U+0045 "E" */
    0xfc, 0x21, 0xf8, 0x43, 0xe0,

    /* U+0046 "F" */
    0xfc, 0x21, 0xf8, 0x42, 0x0,

    /* U+0047 "G" */
    0x39, 0x18, 0x21, 0x85, 0x13, 0x80,

    /* U+0048 "H" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x40,

    /* U+0049 "I" */
    0xfe,

    /* U+004A "J" */
    0xf1, 0x11, 0x19, 0xe0,

    /* U+004B "K" */
    0x8a, 0x6b, 0x38, 0xd2, 0x28, 0x40,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x43, 0xe0,

    /* U+004D "M" */
    0x83, 0x8f, 0x1d, 0x5a, 0xb2, 0x60, 0x80,

    /* U+004E "N" */
    0x87, 0x1a, 0x6d, 0x96, 0x38, 0x40,

    /* U+004F "O" */
    0x38, 0x8a, 0xc, 0x18, 0x28, 0x8e, 0x0,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0xfa, 0x8, 0x0,

    /* U+0051 "Q" */
    0x38, 0x8a, 0xc, 0x18, 0x28, 0x8e, 0x7,

    /* U+0052 "R" */
    0xfa, 0x18, 0x61, 0xfa, 0x28, 0x40,

    /* U+0053 "S" */
    0x74, 0x20, 0xe0, 0xc7, 0xc0,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10, 0x80,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x87, 0x37, 0x80,

    /* U+0056 "V" */
    0x82, 0x89, 0x11, 0x22, 0x85, 0x4, 0x0,

    /* U+0057 "W" */
    0x84, 0x28, 0xc9, 0x29, 0x35, 0x22, 0x98, 0x63,
    0xc, 0x60,

    /* U+0058 "X" */
    0x44, 0xd0, 0xa0, 0x82, 0x89, 0xb1, 0x0,

    /* U+0059 "Y" */
    0x44, 0x88, 0xa1, 0x41, 0x2, 0x4, 0x0,

    /* U+005A "Z" */
    0xfc, 0x21, 0x8, 0x61, 0xf, 0xc0,

    /* U+005B "[" */
    0xea, 0xaa, 0xc0,

    /* U+005C "\\" */
    0x88, 0x84, 0x42, 0x22, 0x10,

    /* U+005D "]" */
    0xd5, 0x55, 0xc0,

    /* U+005E "^" */
    0x66, 0xa9,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0x40,

    /* U+0061 "a" */
    0x70, 0x7f, 0x1f, 0x80,

    /* U+0062 "b" */
    0x82, 0xf, 0xa3, 0x86, 0x3f, 0x80,

    /* U+0063 "c" */
    0x74, 0x21, 0x7, 0x0,

    /* U+0064 "d" */
    0x4, 0x17, 0xe1, 0x86, 0x17, 0x40,

    /* U+0065 "e" */
    0x74, 0x7f, 0x7, 0x0,

    /* U+0066 "f" */
    0x74, 0xf4, 0x44, 0x40,

    /* U+0067 "g" */
    0x7e, 0x38, 0x63, 0x7c, 0x17, 0x80,

    /* U+0068 "h" */
    0x84, 0x3d, 0x18, 0xc6, 0x20,

    /* U+0069 "i" */
    0xbe,

    /* U+006A "j" */
    0x20, 0x92, 0x49, 0xe0,

    /* U+006B "k" */
    0x84, 0x27, 0x4e, 0x4a, 0x20,

    /* U+006C "l" */
    0xfe,

    /* U+006D "m" */
    0xf7, 0x44, 0x62, 0x31, 0x18, 0x88,

    /* U+006E "n" */
    0xf4, 0x63, 0x18, 0x80,

    /* U+006F "o" */
    0x7a, 0x18, 0x61, 0x78,

    /* U+0070 "p" */
    0xfa, 0x38, 0x63, 0xfa, 0x8, 0x0,

    /* U+0071 "q" */
    0x7e, 0x38, 0x61, 0x7c, 0x10, 0x40,

    /* U+0072 "r" */
    0xf2, 0x48,

    /* U+0073 "s" */
    0x7a, 0x1c, 0x17, 0x80,

    /* U+0074 "t" */
    0x4f, 0x44, 0x47,

    /* U+0075 "u" */
    0x8c, 0x63, 0x17, 0x80,

    /* U+0076 "v" */
    0x89, 0x25, 0xc, 0x30,

    /* U+0077 "w" */
    0x88, 0xaa, 0x95, 0x4a, 0xa2, 0x20,

    /* U+0078 "x" */
    0x4b, 0x88, 0xa4, 0x80,

    /* U+0079 "y" */
    0x89, 0x25, 0xc, 0x30, 0x8c, 0x0,

    /* U+007A "z" */
    0xf8, 0x88, 0x8f, 0x80,

    /* U+007B "{" */
    0x69, 0x28, 0x92, 0x60,

    /* U+007C "|" */
    0xff, 0x80,

    /* U+007D "}" */
    0xc9, 0x22, 0x92, 0xc0,

    /* U+007E "~" */
    0xd5, 0x80,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xbc,

    /* U+00A2 "¢" */
    0x23, 0xa9, 0x4a, 0x38, 0x80,

    /* U+00A3 "£" */
    0x3d, 0x4, 0x3e, 0x41, 0xf, 0xc0,

    /* U+00A4 "¤" */
    0xb7, 0x38, 0x73, 0xb4, 0x0,

    /* U+00A5 "¥" */
    0x82, 0x88, 0xa1, 0x47, 0xcf, 0x84, 0x0,

    /* U+00A6 "¦" */
    0xe3, 0x80,

    /* U+00A7 "§" */
    0x74, 0x31, 0x69, 0x38, 0x5c,

    /* U+00A8 "¨" */
    0xc0,

    /* U+00A9 "©" */
    0x38, 0x8a, 0xce, 0x1c, 0x2e, 0x8e, 0x0,

    /* U+00AA "ª" */
    0xff, 0x80,

    /* U+00AB "«" */
    0x55, 0x14,

    /* U+00AC "¬" */
    0xf8, 0x42,

    /* U+00AD "­" */
    0xc0,

    /* U+00AE "®" */
    0x38, 0x8a, 0xfd, 0x3b, 0xac, 0x8e, 0x0,

    /* U+00AF "¯" */
    0xf0,

    /* U+00B0 "°" */
    0x69, 0x60,

    /* U+00B1 "±" */
    0x21, 0x3e, 0x40, 0x7c,

    /* U+00B2 "²" */
    0xe5, 0x70,

    /* U+00B3 "³" */
    0xe8, 0xf0,

    /* U+00B4 "´" */
    0x40,

    /* U+00B5 "µ" */
    0x8c, 0x63, 0x1f, 0xc2, 0x0,

    /* U+00B6 "¶" */
    0x7f, 0x7a, 0x52, 0x94, 0xa5,

    /* U+00B7 "·" */
    0x80,

    /* U+00B8 "¸" */
    0xb0,

    /* U+00B9 "¹" */
    0xc9, 0x70,

    /* U+00BA "º" */
    0xf9, 0x60,

    /* U+00BB "»" */
    0x51, 0x54,

    /* U+00BC "¼" */
    0xc2, 0x22, 0x12, 0x1d, 0x21, 0x29, 0x3c, 0x82,

    /* U+00BD "½" */
    0xc2, 0x22, 0x12, 0x9, 0x6f, 0x11, 0x10, 0x9e,

    /* U+00BE "¾" */
    0xe2, 0x22, 0x19, 0x5, 0x2d, 0x20, 0xbc, 0x82,

    /* U+00BF "¿" */
    0x60, 0x19, 0x8, 0x38,

    /* U+00C0 "À" */
    0x20, 0x20, 0x41, 0x42, 0x88, 0x9f, 0x21, 0x82,

    /* U+00C1 "Á" */
    0x8, 0x20, 0x41, 0x42, 0x88, 0x9f, 0x21, 0x82,

    /* U+00C2 "Â" */
    0x10, 0x50, 0x41, 0x42, 0x88, 0x9f, 0x21, 0x82,

    /* U+00C3 "Ã" */
    0x38, 0x50, 0x41, 0x42, 0x88, 0x9f, 0x21, 0x82,

    /* U+00C4 "Ä" */
    0x28, 0x20, 0xa1, 0x44, 0x4f, 0x90, 0xc1,

    /* U+00C5 "Å" */
    0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x3c, 0x42,
    0x42,

    /* U+00C6 "Æ" */
    0xf, 0xc5, 0x1, 0x40, 0x9f, 0x3c, 0x11, 0x8,
    0x7c,

    /* U+00C7 "Ç" */
    0x3d, 0x18, 0x20, 0x81, 0x13, 0xc4, 0x30,

    /* U+00C8 "È" */
    0x41, 0x3f, 0x8, 0x7e, 0x10, 0xf8,

    /* U+00C9 "É" */
    0x11, 0x3f, 0x8, 0x7e, 0x10, 0xf8,

    /* U+00CA "Ê" */
    0x22, 0xbf, 0x8, 0x7e, 0x10, 0xf8,

    /* U+00CB "Ë" */
    0x57, 0xe1, 0xf, 0xc2, 0x1f,

    /* U+00CC "Ì" */
    0x95, 0x55, 0x40,

    /* U+00CD "Í" */
    0x6a, 0xaa, 0x80,

    /* U+00CE "Î" */
    0x55, 0x24, 0x92, 0x40,

    /* U+00CF "Ï" */
    0xa9, 0x24, 0x92,

    /* U+00D0 "Ð" */
    0x7c, 0x42, 0x41, 0xf1, 0x41, 0x42, 0x7c,

    /* U+00D1 "Ñ" */
    0x29, 0x48, 0x71, 0xa6, 0xd9, 0x63, 0x84,

    /* U+00D2 "Ò" */
    0x20, 0x20, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D3 "Ó" */
    0x8, 0x20, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D4 "Ô" */
    0x10, 0x50, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D5 "Õ" */
    0x38, 0x70, 0xe2, 0x28, 0x30, 0x60, 0xa2, 0x38,

    /* U+00D6 "Ö" */
    0x14, 0x71, 0x14, 0x18, 0x30, 0x51, 0x1c,

    /* U+00D7 "×" */
    0x96, 0x60,

    /* U+00D8 "Ø" */
    0x2, 0x79, 0x14, 0x59, 0x34, 0x59, 0x3c, 0x80,

    /* U+00D9 "Ù" */
    0x40, 0x88, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+00DA "Ú" */
    0x8, 0x48, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+00DB "Û" */
    0x0, 0xc8, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+00DC "Ü" */
    0x32, 0x18, 0x61, 0x86, 0x1c, 0xde,

    /* U+00DD "Ý" */
    0x8, 0x21, 0x12, 0x22, 0x85, 0x4, 0x8, 0x10,

    /* U+00DE "Þ" */
    0x83, 0xe8, 0x61, 0x87, 0xe8, 0x0,

    /* U+00DF "ß" */
    0x74, 0x63, 0x68, 0xc6, 0xc0,

    /* U+00E0 "à" */
    0x60, 0x1c, 0x1f, 0xc7, 0xe0,

    /* U+00E1 "á" */
    0x11, 0x1c, 0x1f, 0xc7, 0xe0,

    /* U+00E2 "â" */
    0x32, 0x1c, 0x1f, 0xc7, 0xe0,

    /* U+00E3 "ã" */
    0x70, 0x1c, 0x1f, 0xc7, 0xe0,

    /* U+00E4 "ä" */
    0x50, 0x1c, 0x1f, 0xc7, 0xe0,

    /* U+00E5 "å" */
    0x32, 0x8c, 0xe0, 0xfe, 0x3f,

    /* U+00E6 "æ" */
    0x77, 0x4, 0x7f, 0xf1, 0x7, 0x70,

    /* U+00E7 "ç" */
    0x74, 0x21, 0x7, 0x11, 0x80,

    /* U+00E8 "è" */
    0x41, 0x1d, 0x1f, 0xc1, 0xc0,

    /* U+00E9 "é" */
    0x11, 0x1d, 0x1f, 0xc1, 0xc0,

    /* U+00EA "ê" */
    0x22, 0x9d, 0x1f, 0xc1, 0xc0,

    /* U+00EB "ë" */
    0x38, 0x1d, 0x1f, 0xc1, 0xc0,

    /* U+00EC "ì" */
    0x95, 0x54,

    /* U+00ED "í" */
    0x6a, 0xa8,

    /* U+00EE "î" */
    0x55, 0x24, 0x90,

    /* U+00EF "ï" */
    0xa1, 0x24, 0x90,

    /* U+00F0 "ð" */
    0x70, 0xe4, 0x5d, 0x8e, 0x37, 0x80,

    /* U+00F1 "ñ" */
    0x70, 0x3d, 0x18, 0xc6, 0x20,

    /* U+00F2 "ò" */
    0x20, 0x7, 0xa1, 0x86, 0x17, 0x80,

    /* U+00F3 "ó" */
    0x10, 0x7, 0xa1, 0x86, 0x17, 0x80,

    /* U+00F4 "ô" */
    0x30, 0x7, 0xa1, 0x86, 0x17, 0x80,

    /* U+00F5 "õ" */
    0x78, 0x7, 0xa1, 0x86, 0x17, 0x80,

    /* U+00F6 "ö" */
    0x28, 0x7, 0xa1, 0x86, 0x17, 0x80,

    /* U+00F7 "÷" */
    0x40, 0xf0, 0x40,

    /* U+00F8 "ø" */
    0x5, 0xe9, 0x69, 0xa5, 0xe8, 0x0,

    /* U+00F9 "ù" */
    0x41, 0x23, 0x18, 0xc5, 0xe0,

    /* U+00FA "ú" */
    0x11, 0x23, 0x18, 0xc5, 0xe0,

    /* U+00FB "û" */
    0x22, 0xa3, 0x18, 0xc5, 0xe0,

    /* U+00FC "ü" */
    0x50, 0x23, 0x18, 0xc5, 0xe0,

    /* U+00FD "ý" */
    0x10, 0x88, 0x92, 0x50, 0xc3, 0x8, 0xc0,

    /* U+00FE "þ" */
    0x82, 0xf, 0xa3, 0x86, 0x3f, 0xa0, 0x80,

    /* U+00FF "ÿ" */
    0x50, 0x8, 0x92, 0x50, 0xc3, 0x8, 0xc0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 43, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 43, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 63, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 3, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 10, .adv_w = 99, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 16, .adv_w = 135, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 110, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 34, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 31, .adv_w = 54, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 34, .adv_w = 54, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 37, .adv_w = 64, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 39, .adv_w = 93, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 42, .adv_w = 36, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 43, .adv_w = 61, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 44, .adv_w = 36, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 56, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 50, .adv_w = 107, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 59, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 92, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 92, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 107, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 92, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 99, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 96, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 103, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 99, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 102, .adv_w = 36, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 36, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 106, .adv_w = 93, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 109, .adv_w = 93, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 111, .adv_w = 93, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 114, .adv_w = 92, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 165, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 131, .adv_w = 117, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 116, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 132, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 107, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 124, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 130, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 50, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 82, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 115, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 190, .adv_w = 95, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 153, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 130, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 134, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 116, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 134, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 228, .adv_w = 116, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 99, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 94, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 127, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 114, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 180, .box_w = 11, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 108, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 104, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 105, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 53, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 290, .adv_w = 56, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 295, .adv_w = 53, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 298, .adv_w = 93, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 300, .adv_w = 80, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 301, .adv_w = 96, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 302, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 306, .adv_w = 109, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 91, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 316, .adv_w = 109, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 56, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 110, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 336, .adv_w = 109, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 45, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 45, .box_w = 3, .box_h = 9, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 346, .adv_w = 99, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 351, .adv_w = 45, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 169, .box_w = 9, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 109, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 102, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 109, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 372, .adv_w = 109, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 378, .adv_w = 66, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 80, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 66, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 387, .adv_w = 108, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 89, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 144, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 88, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 89, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 411, .adv_w = 83, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 56, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 419, .adv_w = 48, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 421, .adv_w = 56, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 425, .adv_w = 93, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 427, .adv_w = 43, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 43, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 429, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 434, .adv_w = 103, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 440, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 445, .adv_w = 113, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 48, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 454, .adv_w = 80, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 459, .adv_w = 96, .box_w = 2, .box_h = 1, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 460, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 467, .adv_w = 65, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 469, .adv_w = 81, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 471, .adv_w = 93, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 473, .adv_w = 61, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 474, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 481, .adv_w = 96, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 482, .adv_w = 67, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 484, .adv_w = 93, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 69, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 490, .adv_w = 69, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 492, .adv_w = 96, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 493, .adv_w = 109, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 498, .adv_w = 104, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 503, .adv_w = 43, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 504, .adv_w = 96, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 505, .adv_w = 69, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 507, .adv_w = 67, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 509, .adv_w = 81, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 511, .adv_w = 165, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 519, .adv_w = 165, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 527, .adv_w = 165, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 92, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 539, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 547, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 555, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 563, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 571, .adv_w = 117, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 578, .adv_w = 117, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 587, .adv_w = 168, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 596, .adv_w = 116, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 603, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 615, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 621, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 626, .adv_w = 50, .box_w = 2, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 50, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 632, .adv_w = 50, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 50, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 133, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 646, .adv_w = 130, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 653, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 669, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 677, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 134, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 692, .adv_w = 93, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 694, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 702, .adv_w = 127, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 127, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 127, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 127, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 729, .adv_w = 104, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 737, .adv_w = 116, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 108, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 748, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 753, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 758, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 763, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 768, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 773, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 158, .box_w = 9, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 784, .adv_w = 91, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 789, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 794, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 799, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 804, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 809, .adv_w = 45, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 811, .adv_w = 45, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 813, .adv_w = 45, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 816, .adv_w = 45, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 819, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 825, .adv_w = 109, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 836, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 842, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 848, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 854, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 860, .adv_w = 93, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 863, .adv_w = 102, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 869, .adv_w = 108, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 874, .adv_w = 108, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 879, .adv_w = 108, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 884, .adv_w = 108, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 889, .adv_w = 89, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 896, .adv_w = 109, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 903, .adv_w = 89, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 96, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 1, 2, 0, 3, 4, 5,
    2, 6, 7, 8, 9, 10, 9, 10,
    11, 12, 0, 13, 14, 15, 16, 17,
    18, 19, 12, 20, 20, 0, 0, 0,
    21, 22, 23, 24, 25, 22, 26, 27,
    28, 29, 29, 30, 31, 32, 29, 29,
    22, 33, 34, 35, 3, 36, 30, 37,
    37, 38, 39, 40, 41, 42, 43, 0,
    44, 0, 45, 46, 47, 48, 49, 50,
    51, 45, 52, 52, 53, 48, 45, 45,
    46, 46, 54, 55, 56, 57, 51, 58,
    58, 59, 58, 60, 41, 0, 0, 9,
    0, 61, 47, 62, 63, 64, 0, 65,
    0, 22, 8, 66, 0, 9, 22, 0,
    67, 0, 0, 0, 0, 51, 29, 9,
    0, 0, 8, 9, 0, 0, 0, 68,
    23, 23, 23, 23, 23, 23, 26, 25,
    26, 26, 26, 26, 29, 29, 29, 29,
    22, 29, 22, 22, 22, 22, 22, 0,
    22, 30, 30, 30, 30, 39, 69, 46,
    45, 45, 45, 45, 45, 45, 49, 47,
    49, 49, 49, 49, 51, 51, 70, 51,
    71, 45, 46, 46, 46, 46, 46, 9,
    46, 51, 51, 51, 51, 58, 46, 58
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, 0, 3, 4, 5,
    2, 6, 7, 8, 9, 10, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 12,
    18, 19, 20, 21, 21, 0, 0, 0,
    22, 23, 24, 25, 23, 25, 25, 25,
    23, 25, 25, 26, 25, 25, 25, 25,
    23, 25, 23, 25, 3, 27, 28, 29,
    29, 30, 31, 32, 33, 34, 35, 0,
    36, 0, 37, 38, 39, 39, 39, 0,
    39, 38, 40, 41, 38, 38, 42, 42,
    39, 42, 39, 42, 43, 44, 45, 46,
    46, 47, 46, 48, 0, 0, 35, 9,
    0, 49, 39, 50, 51, 52, 0, 53,
    0, 23, 8, 9, 9, 9, 23, 0,
    54, 0, 0, 0, 0, 42, 55, 9,
    0, 0, 8, 56, 0, 0, 0, 57,
    24, 24, 24, 24, 24, 24, 24, 23,
    25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 23, 23, 23, 23, 23, 0,
    23, 28, 28, 28, 28, 31, 25, 38,
    37, 37, 37, 37, 37, 37, 37, 39,
    39, 39, 39, 39, 58, 42, 59, 60,
    39, 42, 39, 39, 39, 39, 39, 9,
    39, 45, 45, 45, 45, 46, 38, 46
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 7, 0, 4,
    -4, 0, 0, 0, 0, -9, -10, 1,
    8, 4, 3, -6, 1, 8, 0, 7,
    2, 5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 10, 1, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, 3,
    0, 3, 7, 0, -7, 0, 0, 0,
    0, -5, 0, 0, 0, 0, 0, -3,
    3, 3, 0, 0, -2, 0, -1, 2,
    0, -2, 0, -2, -1, -3, 0, 0,
    0, 0, -2, 0, 0, -2, -2, 0,
    0, -2, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, -2, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    -2, 0, 0, 0, 0, -4, 0, -19,
    0, 0, -3, 0, 3, 5, 0, 0,
    -3, 2, 2, 5, 3, -3, 3, 0,
    0, -9, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, 0, 0, 0, 0, 0,
    -2, -8, 0, -6, -1, 0, 0, 0,
    0, 0, 6, 0, -5, -1, 0, 0,
    0, -3, 0, 0, -1, -12, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -13, -1, 6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, -7, -7, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 5, 0,
    2, 0, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 6, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 3, 2, 5,
    -2, 0, 0, 3, -2, -5, -22, 1,
    4, 3, 0, -2, 0, 6, 0, 5,
    0, 5, 0, -15, 0, -2, 5, 0,
    5, -2, 3, 2, 0, 0, 0, -2,
    0, 0, -3, 13, 0, 13, 0, 5,
    0, 7, 2, 3, 0, 0, 1, 0,
    0, 5, 6, 2, -13, 13, 13, 13,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, -1, 0, 1, -3, -2, -3, 1,
    0, -2, 0, 0, 0, -6, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, -4, 2, 0, -14, -2,
    0, 0, 0, 0, 0, -9, 0, -10,
    0, 0, 0, 0, -1, 0, 16, -2,
    -2, 2, 2, -1, 0, -2, 2, 0,
    0, -8, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -16, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, -5,
    0, -10, -21, 0, 2, 0, 0, 0,
    0, 10, 0, 0, -6, 0, 5, 0,
    -11, -16, -11, -3, 5, 0, 0, -11,
    0, 2, -4, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 4, 5, -20, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -10, -2, 0, 6, -3, 8, 0, -8,
    -12, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 1, 1, -2, -3, 0,
    0, 0, -2, 0, 0, -1, 0, 0,
    0, -3, 0, -1, 0, -4, -3, 0,
    -4, -5, -5, -3, 0, -3, 0, -3,
    0, 0, 0, 0, -1, 0, 0, 2,
    0, 1, -2, 0, 0, 0, 0, -2,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 2, -1, 0, 0, 0,
    -1, 2, 2, 0, 0, 0, 0, -3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, -1, 0, -2, 0, -3, 0,
    0, -1, 0, 5, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 0, -1, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, -1, 0, -2, -2, 0, 0,
    0, 0, 0, 0, 0, 0, -1, 0,
    -2, -2, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, -1, -2, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, -5, -1, -5, 3, 0, 0, -3,
    2, 3, 4, 0, -4, 0, -2, 0,
    0, -8, 2, -1, 1, -8, 2, 0,
    0, 0, -8, 0, -8, -1, -14, -1,
    0, -8, 0, 3, 4, 0, 2, 0,
    0, 0, 0, 0, 0, -3, -2, 0,
    0, 2, 2, -2, 3, -5, -3, 0,
    2, 0, 0, 0, 0, 0, 0, -2,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, -1, -1, 0, -1, -2, 0, 0,
    0, 0, 0, 0, 0, -2, -2, 0,
    -1, -2, -1, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, -1, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, -1, 0, -3, 2, 0, 0, -2,
    1, 2, 2, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 1,
    0, 0, -2, 0, -2, -1, -2, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    -1, 0, 0, 0, 0, -2, -2, 0,
    0, 0, 2, 0, 3, -3, 0, 0,
    0, 0, 0, 0, 0, 5, -1, 0,
    -5, 0, 0, 4, -8, -8, -7, -3,
    2, 0, -1, -10, -3, 0, -3, 0,
    -3, 2, -3, -10, 0, -4, 0, 0,
    1, 0, 1, -1, 0, 2, 0, -5,
    -6, 0, -8, -4, -3, -4, -5, -2,
    -4, 0, -3, -4, -4, -2, -3, 2,
    -2, 1, 4, -3, -10, -4, -4, -4,
    0, 0, 0, -2, 0, 0, 0, 1,
    0, 2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, -1,
    0, 0, -2, 0, -3, -4, -4, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 1, -1, 0,
    0, 0, 0, -2, 2, 0, 0, 0,
    -3, 0, 0, 0, 0, 2, 0, 0,
    0, 0, 0, 0, 0, 0, 8, 0,
    0, 0, 0, 0, 0, 1, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, -3, 0, 0, 0,
    0, -8, -5, 0, 0, 0, -2, -8,
    0, 0, -2, 2, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, 0, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, -2, 0, 2, 0, 2,
    -17, 0, 0, 0, 0, -3, 0, 0,
    0, 0, 2, 0, 1, -3, -3, 0,
    -2, -2, -2, 0, 0, 0, 0, 0,
    0, -5, 0, -2, 0, -2, -2, 0,
    -4, -4, -5, -1, 0, -3, 0, -5,
    0, 0, 0, 0, 13, 0, 0, 1,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 2, 0, 0, -3, 0, 0, 0,
    0, -7, 0, 0, 0, 0, 0, -15,
    -3, 5, 5, -1, -7, 0, 2, -2,
    0, -8, -1, -2, 2, -11, -2, 2,
    0, 2, -6, -2, -6, -5, -7, 0,
    0, -10, 0, 9, 0, 0, -1, 0,
    0, 0, -1, -1, -2, -4, -5, 0,
    2, 0, 0, 0, 0, -15, -13, 2,
    3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, 0,
    -1, -2, -2, 0, 0, -3, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 3,
    0, 2, 0, -4, 2, -1, 0, -4,
    -2, 0, -2, -2, -1, 0, -2, -3,
    0, 0, -1, 0, -1, -3, -2, 0,
    0, -2, 0, 2, -1, 0, -4, 0,
    0, 0, -3, 0, -3, 0, -3, -3,
    -2, 0, 0, 0, -1, 2, 1, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 2, 0, -2,
    0, -1, -2, -5, -1, -1, -1, 0,
    -1, -2, 0, 0, 0, 0, 0, 0,
    -2, -1, -1, 0, 0, 0, 0, 2,
    -1, 0, -1, 0, 0, 0, -1, -2,
    -1, -1, -2, -1, -1, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    1, 6, 0, 0, -4, 0, -1, 3,
    0, -2, -7, -2, 2, 0, 0, -8,
    -3, 2, -3, 1, 0, -1, -1, -5,
    0, -2, 1, 0, 0, -3, 0, 0,
    0, 2, 2, -3, -3, 0, -3, -2,
    -2, -2, -2, 0, -3, 1, -3, -3,
    -2, 0, 0, 0, -1, 5, 2, -2,
    -8, -2, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, -2, 0,
    0, -2, -2, 0, 0, 0, 0, -2,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, -2, 0, -3, 0, 0, 0,
    -5, 0, 1, -4, 3, 0, -1, -8,
    0, 0, -4, -2, 0, -6, -4, -4,
    0, 0, -7, -2, -6, -6, -8, 0,
    -4, 0, 1, 11, -2, 0, -4, -2,
    0, -2, -3, -4, -3, -6, -7, -4,
    0, 0, 0, 0, 0, -2, -1, 0,
    0, -2, -2, -2, 0, 0, -1, 0,
    0, 0, 0, -11, -1, 5, 4, -4,
    -6, 0, 0, -5, 0, -8, -1, -2,
    3, -15, -2, 0, 0, 0, -10, -2,
    -8, -2, -12, 0, 0, -11, 0, 9,
    0, 0, -1, 0, 0, 0, 0, -1,
    -1, -6, -1, 0, 0, 0, 0, 0,
    0, -10, -12, 1, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, -1, 0,
    0, -4, -8, 0, 0, -1, -2, -5,
    -2, 0, -1, 0, 0, 0, 0, -7,
    -2, -5, -5, -1, -3, -4, -2, -3,
    0, -3, -1, -5, -2, 0, -2, -3,
    -2, -3, 0, 1, 0, -1, -5, 0,
    0, 0, 0, 0, 0, 3, 0, 0,
    -8, -3, -3, -3, 0, -3, 0, 0,
    0, 0, 2, 0, 1, -3, 7, 0,
    -2, -2, -2, 0, 0, 0, 0, 0,
    0, -5, 0, -2, 0, -2, -2, 0,
    -4, -4, -5, -1, 0, -3, 1, 6,
    0, 0, 0, 0, 13, 0, 0, 1,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 2, 0, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, -3,
    0, 0, 0, 0, 0, -1, 0, 0,
    0, -2, -2, 0, 0, -3, -2, 0,
    0, -3, 0, 3, -1, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 0, 0,
    -2, 0, 0, 0, 3, 1, -1, 0,
    -5, -3, 0, 5, -5, -5, -3, -3,
    6, 3, 2, -14, -1, 3, -2, 0,
    -2, 2, -2, -6, 0, -2, 2, -2,
    -1, -5, -1, 0, 0, 5, 3, 0,
    -4, 0, -9, -2, 5, -2, -6, 0,
    -2, -5, -5, -2, -3, 0, 0, 0,
    0, 6, 5, -2, -9, 2, 5, 3,
    2, 0, -2, 0, -4, 0, 1, 5,
    -4, -6, -6, -4, 5, 0, 0, -12,
    -1, 2, -3, -1, -4, 0, -4, -6,
    -2, -2, -1, 0, 0, -4, -3, -2,
    0, 5, 4, -2, -9, 0, -9, -2,
    0, -6, -9, 0, -5, -3, -5, -4,
    -5, 0, 0, 0, 0, 4, 4, -2,
    -12, 3, 6, 3, 0, 0, -2, 0,
    -3, -1, 0, -2, -3, 0, 3, -5,
    2, 0, 0, -8, 0, -2, -4, -3,
    -1, -5, -4, -5, -4, 0, -5, -2,
    -4, -3, -5, -2, 0, 0, 0, 8,
    -3, 0, -5, -2, 0, -2, -3, -4,
    -4, -4, -6, -2, -2, 0, 0, 0,
    0, -3, 0, -1, 0, -2, -2, -2,
    3, 0, -2, 0, -8, -2, 1, 3,
    -5, -6, -3, -5, 5, -2, 1, -15,
    -3, 3, -4, -3, -6, 0, -5, -7,
    -2, -2, -1, -2, -3, -5, 0, 0,
    0, 5, 4, -1, -10, 0, -10, -4,
    4, -6, -11, -3, -6, -7, -8, -5,
    -6, 0, 0, 0, -3, 3, 4, -3,
    -13, 5, 5, 4, 0, 0, 0, 0,
    -2, 0, 0, 2, -2, 3, 1, -3,
    3, 0, 0, -5, 0, 0, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 1, 5,
    0, 0, -2, 0, 0, 0, 0, -1,
    -1, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 1, 0,
    -1, 0, 6, 0, 3, 0, 0, -2,
    0, 3, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 5, 0, 4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -10, 0, -2,
    3, 0, 5, 0, 0, 16, 2, -3,
    -3, 2, 2, -1, 0, -8, 0, 0,
    8, -10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -11, 6, 22,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 6, 0, 0, 0,
    5, -10, -5, 1, 5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -3, 0, 0, -3, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, 0, -4,
    0, 0, 0, 0, 0, 2, 21, -3,
    -1, 5, 4, -4, 2, 0, 0, 2,
    2, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -21, 4, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 2, 3, -1, -2,
    4, 0, -11, 2, 0, 0, 0, 0,
    0, 0, 0, -4, 0, 0, 0, -4,
    0, 0, 0, 0, -4, -1, 0, 0,
    0, -4, 0, -2, 0, -8, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -11, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, -3, -4, 0,
    0, 0, 0, 0, 0, -2, 0, -4,
    0, 0, 0, -3, 2, -2, 0, 0,
    -4, -2, -4, 0, 0, -4, 0, -2,
    0, -8, 0, -2, 0, 0, -13, -3,
    -6, -2, -6, 0, 0, -11, 0, -4,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, -2, -3, -1, 0, 0, 0, 0,
    0, -3, -4, 0, -2, 0, 0, 0,
    0, 0, 0, 0, -4, 0, -4, 2,
    -2, 3, 0, -1, -4, -1, -3, -3,
    0, -2, -1, -1, 1, -4, 0, 0,
    0, 0, -14, -1, -2, 0, -4, 0,
    -1, -8, -1, 0, 0, -1, -1, 0,
    0, 0, 0, 1, 0, -1, -3, -1,
    0, 0, 0, 0, -1, 3, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, -1, 0, 0, 0, -3,
    2, 0, 0, 0, -4, -2, -3, 0,
    0, -4, 0, -2, 0, -8, 0, 0,
    0, 0, -16, 0, -3, -6, -8, 0,
    0, -11, 0, -1, -2, 0, 0, 0,
    0, 0, 0, 0, 0, -2, -2, -1,
    0, 0, 0, 0, 0, -2, -3, 0,
    -2, 0, 0, 0, 0, 0, 0, 3,
    -2, 0, 5, 8, -2, -2, -5, 2,
    8, 3, 4, -4, 2, 7, 2, 5,
    4, 4, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 10, 8, -3,
    -2, 0, -1, 13, 7, 13, 0, 0,
    0, 2, 0, 0, 0, 0, 0, 0,
    0, 6, 7, 0, -2, 11, 8, 10,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 0, 0,
    0, 0, -13, -2, -1, -7, -8, 0,
    0, -11, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 0, 0, 0, 0, -13, -2,
    -1, -7, -8, 0, 0, -6, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    -4, 2, 0, -2, 1, 3, 2, -5,
    0, 0, -1, 2, 0, 1, 0, 0,
    0, 0, -4, 0, -1, -1, -3, 0,
    -1, -6, 0, 10, -2, 0, -4, -1,
    0, -1, -3, 0, -2, -4, -3, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, -1, -1, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 0, 0, 0, 0, -13, -2,
    -1, -7, -8, 0, 0, -11, 0, 0,
    0, 0, 0, 0, 8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -3, 0, -5, -2, -1, 5,
    -1, -2, -6, 0, -1, 0, -1, -4,
    0, 4, 0, 1, 0, 1, -4, -6,
    -2, 0, -6, -3, -4, -7, -6, 0,
    -3, -3, -2, -2, -1, -1, -2, -1,
    0, -1, 0, 2, 0, 2, -1, 0,
    0, 0, 0, 0, 2, 5, 5, 0,
    -6, -1, -1, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, -2, -2, 0, 0, -4, 0, -1,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -10, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, -2, 0, 0, 0, 0, 0,
    0, -2, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, -3,
    -2, 2, 0, -3, -3, -1, 0, -5,
    -1, -4, -1, -2, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -11, 0, 5, 0, 0, -3, 0,
    0, 0, 0, -2, 0, -2, 0, 0,
    0, 0, 0, 0, -1, -1, -3, 0,
    0, 0, 0, 0, 0, 0, -1, 0,
    -4, 0, 0, 7, -2, -5, -5, 1,
    2, 2, 0, -4, 1, 2, 1, 5,
    1, 5, -1, -4, 0, 0, -6, 0,
    0, -5, -4, 0, 0, -3, 0, -2,
    -3, 0, -2, 0, -2, 0, -1, 2,
    0, -1, -5, -2, 0, 0, 0, 0,
    0, 6, 5, 0, -5, 0, 0, 0,
    0, 0, -1, 0, -3, 0, 0, 2,
    -4, 0, 2, -2, 1, 0, 0, -5,
    0, -1, 0, 0, -2, 2, -1, 0,
    0, 0, -7, -2, -4, 0, -5, 0,
    0, -8, 0, 6, -2, 0, -3, 0,
    1, 0, -2, 0, -2, -5, 0, -2,
    0, 0, 0, 0, 0, 2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 2, -2, 0, 0, 0,
    -2, -1, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -10, 0, 4,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, -2, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 6, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -9, 0, 2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, 0,
    -6, 0, 0, 4, -5, 0, 2, -6,
    3, -2, -2, -5, -2, 1, -5, -3,
    -5, 0, -2, -7, 0, -3, 0, 0,
    0, -2, 2, 0, 0, 3, 0, 3,
    -6, 0, -5, -3, -4, -3, -6, -3,
    -4, -3, -4, -6, -4, 0, 0, 0,
    -2, 0, 2, -2, -4, -3, 4, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, -2, 0, 0, 0, -1, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, 2, 3, 0, 0,
    0, 0, 2, 0, -4, -5, -5, -2,
    5, 0, 2, -2, 0, 4, -2, 0,
    -6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 8, 2, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, 0, -6, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 4, 0, 0, 0, 0, 3,
    0, 1, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 1, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -9, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 3, 0, 4, 0, 0, 0, 0,
    0, -10, -9, 0, 7, 5, 3, -6,
    1, 7, 0, 6, 0, 3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 8, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -10, 0, 0, 0, 0, -5, 0, -10,
    -3, 0, 0, 0, -6, 0, 6, -8,
    -7, 0, 0, -7, 0, -7, -7, 0,
    0, -21, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -15, 1, 8,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, -3, -8,
    0, -9, -14, -2, -4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, -5, 0, -3, -4, -3, 0,
    -2, 0, 0, 0, 0, -5, 0, -5,
    0, -6, -4, 0, -2, -5, -5, -3,
    0, -6, 0, -5, -2, 0, 0, 0,
    -2, 0, 0, 1, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -4, 0, 0, 0, 0, 0, -3, 0,
    0, 0, 0, 13, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, 6, 0, 0, 0, 0, -13, -2,
    -1, -7, -8, 0, 0, -11, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 2,
    0, -3, 0, 1, 0, 0, -1, 0,
    0, 0, 0, 0, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 3, 0, 0,
    -1, 0, 0, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 71,
    .right_class_cnt     = 60,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t scopebuddy_font_10 = {
#else
lv_font_t scopebuddy_font_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 11,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if SCOPEBUDDY_FONT_10*/

