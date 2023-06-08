#include <tui++/Char.h>

#include <cassert>

using namespace tui;

constexpr Char SP { ' ' };

void test_Char() {
  auto sp = SP;

  assert(sp == SP);
}
