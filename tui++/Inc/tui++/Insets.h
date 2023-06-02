#pragma once

namespace tui {

/**
 * The Insets class specifies the blank space that must be left around the inside of the edges of a Container.
 * The space can be used for a border, a title, or other items (such as a scrollbar).
 */
struct Insets {
  int top, left, bottom, right;
};

}

