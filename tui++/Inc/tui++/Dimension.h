#pragma once

namespace tui {

struct Dimension {
  int width, height;

  constexpr bool operator==(const Dimension &other) const {
    return this->width == other.width and this->height == other.height;
  }

  constexpr bool operator!=(const Dimension &other) const {
    return this->width != other.width or this->height != other.height;
  }
};

}
