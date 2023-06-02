#pragma once

#include <tui++/Insets.h>

namespace tui {

class Graphics;
class Component;

class Border {
public:
  virtual ~Border() {
  }

  /**
   * Returns the insets of the border.
   */
  Insets get_border_insets(const Component &component);

  /**
   * Paints the border for the specified component with the specified position and size.
   */
  void paint_border(const Component &component, Graphics &g, int x, int y, int width, int height);
};

}
