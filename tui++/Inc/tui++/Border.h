#pragma once

#include <tui++/Theme.h>
#include <tui++/Insets.h>

namespace tui {

class Graphics;
class Component;

class Border: public Themable {
public:
  /**
   * Returns the insets of the border.
   */
  virtual Insets get_border_insets(const Component &c) const = 0;

  /**
   * Paints the border for the specified component with the specified position and size.
   */
  virtual void paint_border(const Component &c, Graphics &g, int x, int y, int width, int height) const = 0;
};

}
