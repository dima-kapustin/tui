#include <tui++/Char.h>

using namespace tui;

constexpr Char SP = ' ';

static_assert(SP.glyph_width() == 1);

void test_Char() {
  constexpr auto sp = SP;

  static_assert(sp == SP);
}
