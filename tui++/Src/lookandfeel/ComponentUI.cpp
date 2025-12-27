#include <tui++/lookandfeel/ComponentUI.h>

#include <tui++/Graphics.h>
#include <tui++/Component.h>

namespace tui::laf {

ComponentUI::~ComponentUI() {
}

void ComponentUI::install_ui(std::shared_ptr<Component> const &c) {
}

void ComponentUI::uninstall_ui(std::shared_ptr<Component> const &c) {
}

void ComponentUI::update(Graphics &g, std::shared_ptr<const Component> const &c) const {
  if (c->is_opaque()) {
    auto &&background_color = c->get_background_color();
    g.set_background_color(background_color ? background_color.value() : DEFAULT_COLOR);
    g.fill_rect(c->get_bounds() - c->get_insets());
  }
  paint(g, c);
}

Dimension ComponentUI::get_preferred_size(std::shared_ptr<const Component> const &c) const {
  return Dimension::zero();
}

Dimension ComponentUI::get_minimum_size(std::shared_ptr<const Component> const &c) const {
  return get_preferred_size(c);
}

Dimension ComponentUI::get_maximum_size(std::shared_ptr<const Component> const &c) const {
  return get_preferred_size(c);
}

bool ComponentUI::contains(std::shared_ptr<const Component> const &c, int x, int y) const {
  return x >= 0 and x < c->get_width() and y >= 0 and y < c->get_height();
}

void ComponentUI::paint(Graphics &g, std::shared_ptr<const Component> const &c) const {
}

}

