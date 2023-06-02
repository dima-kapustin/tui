#pragma once

namespace tui {

class Component;
class Graphics;

class Border {
public:
  virtual ~Border() {
  }

  /**
   * Returns the insets of the border.
   */
  Insets get_border_insets(Component &component);

  /**
   * Paints the border for the specified component with the specified position and size.
   */
  void paint_border(Component &component, Graphics &g, int x, int y, int width, int height);
};

}
