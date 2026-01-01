#pragma once

#include <limits>

namespace tui {

struct Dimension {
  int width, height;

  static constexpr Dimension max() {
    return {std::numeric_limits<short>::max(), std::numeric_limits<short>::max()};
  }

  static constexpr Dimension zero() {
    return {0, 0};
  }

  constexpr Dimension& operator=(Dimension const&) = default;

  constexpr bool operator==(const Dimension &other) const {
    return this->width == other.width and this->height == other.height;
  }

  constexpr bool empty() const {
    return this->width <= 0 or this->height <= 0;
  }

  constexpr operator bool() const {
    return not empty();
  }
};

}
