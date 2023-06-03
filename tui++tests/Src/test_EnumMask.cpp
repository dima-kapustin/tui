#include <tui++/EnumMask.h>

#include <iostream>

using namespace tui;

enum class Flag {
  A = 1,
  B = 8,
  C = 32,
};

using Flags = EnumMask<Flag>;

void test_EnumMask() {
  auto flags = Flag::A | Flag::C;

  for (auto &&flag : flags) {
    switch (flag) {
    case Flag::A:
      std::cout << "A";
      break;
    case Flag::B:
      std::cout << "B";
      break;
    case Flag::C:
      std::cout << "C";
      break;
    }
  }

  std::cout << std::endl;
}
