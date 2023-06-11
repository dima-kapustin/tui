#include <tui++/Panel.h>
#include <tui++/Color.h>
#include <tui++/Graphics.h>
#include <tui++/FlowLayout.h>

namespace tui {

Panel::Panel() :
    Panel(std::make_shared<FlowLayout>()) {
}

void Panel::paint(Graphics &g) {
  if (auto parent = get_parent()) {
    if (this->background_color.has_vlaue() and this->background_color != parent->get_background_color()) {
      g.set_background_color(this->background_color.value());
      g.fill_rect(0, 0, get_width(), get_height());
    }
  }
  base::paint(g);
}

}
