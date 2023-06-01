#pragma once

namespace tui {

struct Point {
  int x, y;

  constexpr bool operator==(const Point &other) const {
    return this->x == other.x and this->y == other.y;
  }

  constexpr bool operator!=(const Point &other) const {
    return this->x != other.x or this->y != other.y;
  }
};

}
