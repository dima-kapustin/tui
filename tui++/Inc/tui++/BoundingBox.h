#pragma once

namespace tui {

struct Rectangle;

struct BoundingBox {
  int left, right;
  int top, bottom;

  constexpr operator Rectangle() const;
};

}

#include <tui++/Rectangle.h>
