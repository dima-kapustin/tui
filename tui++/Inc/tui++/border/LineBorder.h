#pragma once

#include <tui++/Color.h>
#include <tui++/Border.h>
#include <tui++/Stroke.h>

namespace tui {

class LineBorder: public Border {
  Stroke const stroke;
  Color const color;

public:
  LineBorder(Stroke const &stroke, Color const &color) :
      stroke(stroke), color(color) {
  }

  virtual Insets get_border_insets(const Component &c) const override;

  virtual void paint_border(const Component &c, Graphics &g, int x, int y, int width, int height) const override;
};

}
