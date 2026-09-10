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
    // Like Swing's ComponentUI.update: an opaque component's face fills its
    // own coordinates. The context a component paints through is translated
    // to its origin (Component::paint_children creates it for a child, the
    // screen's get_graphics for a window), while the component's bounds are
    // expressed in its parent's -- filling them here would offset the face
    // by the component's location a second time and, clipped to the
    // component, paint nothing at all. The border is painted over the face
    // (Component::paint runs paint_border after this).
    g.set_background_color(c->get_background_color());
    g.fill_rect(0, 0, c->get_width(), c->get_height());
  }
  paint(g, c);
}

std::optional<Dimension> ComponentUI::get_preferred_size(std::shared_ptr<const Component> const &c) const {
  return std::nullopt;
}

std::optional<Dimension> ComponentUI::get_minimum_size(std::shared_ptr<const Component> const &c) const {
  return std::nullopt;
}

std::optional<Dimension> ComponentUI::get_maximum_size(std::shared_ptr<const Component> const &c) const {
  return std::nullopt;
}

bool ComponentUI::contains(std::shared_ptr<const Component> const &c, int x, int y) const {
  return x >= 0 and x < c->get_width() and y >= 0 and y < c->get_height();
}

void ComponentUI::paint(Graphics &g, std::shared_ptr<const Component> const &c) const {
}

}

