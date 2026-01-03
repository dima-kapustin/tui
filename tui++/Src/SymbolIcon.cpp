#include <tui++/SymbolIcon.h>

#include <tui++/Graphics.h>

namespace tui {

void SymbolIcon::paint_icon(Component const *c, Graphics &g, int x, int y) const {
  g.draw_char(this->symbol, x, y);
}

int SymbolIcon::get_icon_width() const {
  return 1;
}

int SymbolIcon::get_icon_height() const {
  return 1;
}

}
