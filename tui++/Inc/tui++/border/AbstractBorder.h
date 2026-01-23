#pragma once

#include <tui++/Border.h>

namespace tui {

class AbstractBorder: public Border {
public:
  virtual Insets get_border_insets(const Component &c) const override {
    return {};
  }

  virtual void paint_border(const Component &c, Graphics &g, int x, int y, int width, int height) const override {
  }

  virtual bool is_border_opaque() const override {
    return false;
  }
};

}
