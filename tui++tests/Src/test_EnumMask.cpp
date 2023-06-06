#include <tui++/util/EnumMask.h>

#include <cassert>

using namespace tui::util;

enum class Flag {
  A = 1,
  B = 8,
  C = 32,
};

using Flags = EnumMask<Flag>;

void test_EnumMask() {
  auto a = Flag::A | Flag::C;

  Flags b = Flags::NONE;
  for (auto &&flag : a) {
    b |= flag;
  }

  assert(a == b);
}
