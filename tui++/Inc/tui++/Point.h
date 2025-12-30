#pragma once

namespace tui {

struct Point {
  int x, y;

  constexpr bool operator==(Point const &other) const {
    return this->x == other.x and this->y == other.y;
  }

  constexpr Point& operator=(Point const&) = default;
};

}
