#pragma once

#include <tui++/util/EnumFlags.h>

namespace tui {

enum class Attribute {
  NONE = 0,
  STANDOUT = 1 << 0,
  BOLD = 1 << 1,
  DIM = 1 << 2,
  ITALIC = 1 << 3,
  UNDERLINE = 1 << 4,
  BLINK = 1 << 5,
  INVERSE = 1 << 6,
  INVISIBLE = 1 << 7,
  CROSSED_OUT = 1 << 8,
  DOUBLE_UNDERLINE = 1 << 9,
};

using Attributes = util::EnumFlags<Attribute>;

}
