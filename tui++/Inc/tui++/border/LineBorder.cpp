#include <tui++/border/LineBorder.h>

#include <tui++/Graphics.h>

namespace tui {

Insets LineBorder::get_border_insets(const Component &c) const {
  return {1, 1, 1, 1};
}

void LineBorder::paint_border(const Component &c, Graphics &g, int x, int y, int width, int height) const {
  g.set_stroke(this->stroke);
  g.set_foreground_color(this->color);
  g.draw_rect(x, y, width, height);
}

}
