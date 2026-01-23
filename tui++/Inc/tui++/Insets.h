#pragma once

#include <tui++/Theme.h>

namespace tui {

/**
 * The Insets class specifies the blank space that must be left around the inside of the edges of a Container.
 * The space can be used for a border, a title, or other items (such as a scrollbar).
 */
struct Insets: public Themable {
  int top, left, bottom, right;

  constexpr Insets() = default;

  constexpr Insets(int top, int left, int bottom, int right) :
      top(top), left(left), bottom(bottom), right(right) {
  }

  constexpr bool operator==(Insets const &other) const {
    return this->top == other.top and this->left == other.left and this->bottom == other.bottom and this->right == other.right;
  }
};

}

