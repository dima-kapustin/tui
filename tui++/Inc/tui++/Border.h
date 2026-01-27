#pragma once

#include <tui++/Insets.h>
#include <tui++/Themable.h>

namespace tui {

class Graphics;
class Component;

class Border: public Themable {
public:
  virtual Insets get_border_insets(Component const &c) const = 0;

  virtual void paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const = 0;

  virtual bool is_border_opaque() const = 0;
};

}
