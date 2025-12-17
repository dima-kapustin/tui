#include <tui++/lookandfeel/ComponentUI.h>

#include <tui++/Graphics.h>
#include <tui++/Component.h>

namespace tui::laf {

void ComponentUI::update(std::shared_ptr<Graphics> const &g, std::shared_ptr<const Component> const &c) const {
  if (c->is_opaque()) {
    g->set_background_color(c->get_background_color());
    g->fill_rect(0, 0, c->get_width(), c->get_height());
  }
  paint(g, c);
}

bool ComponentUI::contains(std::shared_ptr<const Component> const &c, int x, int y) const {
  return x >= 0 and x < c->get_width() and y >= 0 and y < c->get_height();
}

}

