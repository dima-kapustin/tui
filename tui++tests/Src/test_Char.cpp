#include <tui++/Char.h>

#include <cassert>

using namespace tui;

constexpr Char SP = ' ';

static_assert(SP.glyph_width() == 1);

void test_Char() {
  auto sp = SP;

  assert(sp == SP);

  Char down_and_left_arc = CharCode::DOWN_AND_LEFT_ARC;
  assert(down_and_left_arc == BoxDrawing::DOWN_AND_LEFT_ARC);
}
