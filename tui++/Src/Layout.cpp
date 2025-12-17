#include <tui++/Layout.h>
#include <tui++/Component.h>

namespace tui {

void AbstractLayout::add_layout_component(const std::shared_ptr<Component> &c, const std::any &constraints) {
}

void AbstractLayout::remove_layout_component(const std::shared_ptr<Component> &c) {
}

float AbstractLayout::get_layout_alignment_x(const std::shared_ptr<const Component> &target) const {
  return Component::CENTER_ALIGNMENT;
}

float AbstractLayout::get_layout_alignment_y(const std::shared_ptr<const Component> &target) const {
  return Component::CENTER_ALIGNMENT;
}

void AbstractLayout::invalidate_layout(const std::shared_ptr<const Component> &target) {

}

}
