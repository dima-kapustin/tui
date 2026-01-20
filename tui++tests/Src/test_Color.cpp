#include <tui++/Color.h>

using namespace tui;

void test_Color() {
  static_assert(0x000000_rgb == BLACK_COLOR);
  static_assert(0xffffff_rgb == WHITE_COLOR);
  static_assert(0x808000_rgb == YELLOW_COLOR);
  static_assert(0xffff00_rgb == BRIGHT_YELLOW_COLOR);
  static_assert(0xff'ff'00_rgb == BRIGHT_YELLOW_COLOR);
  static_assert("#ffff00"_rgb == BRIGHT_YELLOW_COLOR);
}
