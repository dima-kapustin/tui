#include <tui++/border/CompoundBorder.h>

namespace tui {

Insets CompoundBorder::get_border_insets(Component const &c) const {
  auto insets = Insets { };
  if (this->outside_border) {
    insets += this->outside_border->get_border_insets(c);
  }
  if (this->inside_border) {
    insets += this->inside_border->get_border_insets(c);
  }
  return insets;
}

void CompoundBorder::paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  auto px = x;
  auto py = y;
  auto pw = width;
  auto ph = height;

  if (this->outside_border) {
    this->outside_border->paint_border(c, g, px, py, pw, ph);

    auto insets = this->outside_border->get_border_insets(c);
    px += insets.left;
    py += insets.top;
    pw = pw - insets.width();
    ph = ph - insets.height();
  }

  if (this->inside_border) {
    this->inside_border->paint_border(c, g, px, py, pw, ph);
  }
}

bool CompoundBorder::is_border_opaque() const {
  return (not this->outside_border or this->outside_border->is_border_opaque()) and //
      (not this->inside_border or this->inside_border->is_border_opaque());
}

}
