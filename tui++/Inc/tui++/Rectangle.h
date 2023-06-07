#pragma once

#include <tui++/Point.h>
#include <tui++/Dimension.h>
#include <tui++/BoundingBox.h>

#include <algorithm>

namespace tui {

struct Rectangle: Point, Dimension {
  constexpr bool contains(int x, int y) const {
    if ((this->width | this->height) < 0) {
      return false;
    }

    if (x < this->x or y < this->y) {
      return false;
    }

    auto right = this->width + x;
    auto bottom = this->height + y;

    return ((right < this->x or right > x) and (bottom < this->y or bottom > y));
  }

  constexpr bool contains(int x, int y, int width, int height) const {
    if ((this->width | this->height | width | height) < 0) {
      // At least one of the dimensions is negative...
      return false;
    }

    // Note: if any dimension is zero, tests below must return false...
    if (x < this->x or y < this->y) {
      return false;
    }

    auto right = this->width + this->x;

    width += x;
    if (width <= x) {
      if (right >= this->x or width > right) {
        return false;
      }
    } else {
      if (right >= this->x and width > right) {
        return false;
      }
    }

    auto bottom = this->height + this->y;
    height += y;
    if (height <= y) {
      if (bottom >= this->y or height > bottom) {
        return false;
      }
    } else {
      if (bottom >= this->y and height > bottom) {
        return false;
      }
    }

    return true;
  }

  constexpr bool operator==(const Rectangle &other) const {
    return this->x == other.x and this->y == other.y and this->width == other.width and this->height == other.height;
  }

  constexpr bool operator!=(const Rectangle &other) const {
    return this->x != other.x or this->y != other.y or this->width != other.width or this->height != other.height;
  }

  constexpr int get_left() const {
    return this->x;
  }

  constexpr int get_top() const {
    return this->y;
  }

  constexpr int get_right() const {
    return this->x + this->width;
  }

  constexpr int get_bottom() const {
    return this->y + this->height;
  }

  // Intersection
  constexpr Rectangle get_intersection(int x, int y, int width, int height) const {
    Rectangle other { x, y, width, height };
    return *this & other;
  }

  constexpr Rectangle& operator&=(const Rectangle &other) {
    auto tx1 = this->x;
    auto ty1 = this->y;
    auto rx1 = other.x;
    auto ry1 = other.y;

    auto tx2 = tx1 + this->width;

    auto ty2 = ty1 + this->height;
    auto rx2 = rx1 + other.width;
    auto ry2 = ry1 + other.height;

    if (tx1 < rx1) {
      tx1 = rx1;
    }
    if (ty1 < ry1) {
      ty1 = ry1;
    }
    if (tx2 > rx2) {
      tx2 = rx2;
    }
    if (ty2 > ry2) {
      ty2 = ry2;
    }

    this->x = tx1;
    this->y = ty1;
    this->width = tx2 - tx1;
    this->height = ty2 - ty1;
    return *this;
  }

  constexpr Rectangle operator&(const Rectangle &other) const {
    auto result = *this;
    result &= other;
    return result;
  }

  // Union
  constexpr Rectangle get_union(int x, int y, int width, int height) const {
    Rectangle other { x, y, width, height };
    return *this | other;
  }

  constexpr Rectangle& operator|=(const Rectangle &other) {
    auto x1 = std::min(this->x, other.x);
    auto x2 = std::max(this->x + this->width, other.x + other.width);
    auto y1 = std::min(this->y, other.y);
    auto y2 = std::max(this->y + this->height, other.y + other.height);

    this->x = x1;
    this->y = y1;
    this->width = x2 - x1;
    this->height = y2 - y1;
    return *this;
  }

  constexpr Rectangle operator|(const Rectangle &other) const {
    auto result = *this;
    result |= other;
    return result;
  }

  void set_bounds(int x, int y, int width, int height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
  }

  constexpr void translate(int dx, int dy) {
    this->x += dx;
    this->y += dy;
  }

  constexpr operator BoundingBox() const {
    return {get_left(), get_right(), get_top(), get_bottom()};
  }
};

constexpr BoundingBox::operator Rectangle() const {
  return {this->left, this->top, this->right - this->left, this->bottom - this->top};
}

}
