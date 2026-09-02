#include <tui++/terminal/graphic/Font16x32.h>

using namespace tui;
using namespace tui::detail;

// The raster must fill the 16x32 graphic cell. Each row is 16 bits wide and
// stored as 2 bytes (bit 7 of byte 0 the leftmost pixel), so every glyph row
// occupies two consecutive table entries.
static_assert(FONT_WIDTH == 16);
static_assert(FONT_HEIGHT == 32);
static_assert(sizeof(FONT16X32_BASIC) / sizeof(FONT16X32_BASIC[0]) == 128);
static_assert(sizeof(FONT16X32_BASIC[0]) == FONT_HEIGHT * 2);

// The stems of the bowl letters must sit on the correct sides (a swapped
// b/d or p/q pair makes the letters look horizontally reflected).
static_assert(FONT16X32_BASIC['b'][10] == 0x38 and FONT16X32_BASIC['b'][11] == 0x00, "b ascender must be on the left");
static_assert(FONT16X32_BASIC['d'][10] == 0x00 and FONT16X32_BASIC['d'][11] == 0x38, "d ascender must be on the right");
static_assert(FONT16X32_BASIC['p'][44] == 0x38 and FONT16X32_BASIC['p'][45] == 0x00, "p descender must be on the left");
static_assert(FONT16X32_BASIC['q'][44] == 0x00 and FONT16X32_BASIC['q'][45] == 0x38, "q descender must be on the right");

// E and F must keep their middle bars.
static_assert(FONT16X32_BASIC['E'][26] == 0x3F and FONT16X32_BASIC['E'][27] == 0xE0, "E must have a middle bar");
static_assert(FONT16X32_BASIC['F'][26] == 0x3F and FONT16X32_BASIC['F'][27] == 0xE0, "F must have a middle bar");

// Cascadia-style details: single-story 'a' (no ascender top), dotted '0',
// dotted 'i', barred 'I', flat-topped 'l'.
static_assert(FONT16X32_BASIC['a'][16] == 0x00 and FONT16X32_BASIC['a'][17] == 0x00, "a must have a single story (no ascender top)");
static_assert(FONT16X32_BASIC['0'][26] != FONT16X32_BASIC['0'][24] or FONT16X32_BASIC['0'][27] != FONT16X32_BASIC['0'][25], "0 must have a dot");
static_assert(FONT16X32_BASIC['i'][12] == 0x03 and FONT16X32_BASIC['i'][13] == 0x80, "i must have a square dot");
static_assert(FONT16X32_BASIC['I'][12] == 0x1F and FONT16X32_BASIC['I'][13] == 0xF8, "I must be barred");
static_assert(FONT16X32_BASIC['l'][10] == 0x3F and FONT16X32_BASIC['l'][11] == 0x00, "l must have a flat top");

void test_Font() {
}
