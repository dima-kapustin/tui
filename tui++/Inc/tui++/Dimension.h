#pragma once

#include <limits>

namespace tui {

struct Dimension {
  int width, height;

  static constexpr Dimension max() {
    return {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
  }

  static constexpr Dimension zero() {
    return {0, 0};
  }

  constexpr bool operator==(const Dimension &other) const {
    return this->width == other.width and this->height == other.height;
  }

  constexpr bool operator!=(const Dimension &other) const {
    return this->width != other.width or this->height != other.height;
  }

  constexpr bool empty() const {
    return not (this->width or this->height);
  }

  constexpr operator bool() const {
    return not empty();
  }
};

}
