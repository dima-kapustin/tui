#pragma once

#include <tui++/Theme.h>
#include <tui++/Insets.h>

#include <optional>

namespace tui {

class Graphics;
class Component;

class Border: public Themable {
public:
  virtual Insets get_border_insets(const Component &c) const = 0;

  virtual void paint_border(const Component &c, Graphics &g, int x, int y, int width, int height) const = 0;

  virtual bool is_border_opaque() const = 0;
};

}
