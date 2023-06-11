#include <tui++/Layout.h>
#include <tui++/Component.h>

namespace tui {

void LayoutBase::add_layout_component(const std::shared_ptr<Component> &component, const std::any &constraints) {
}

void LayoutBase::remove_layout_component(const std::shared_ptr<Component> &component) {
}

float LayoutBase::get_layout_alignment_x(const std::shared_ptr<const Component> &component) const {
  return Component::CENTER_ALIGNMENT;
}

float LayoutBase::get_layout_alignment_y(const std::shared_ptr<const Component> &component) const {
  return Component::CENTER_ALIGNMENT;
}

void LayoutBase::invalidate_layout(const std::shared_ptr<const Component> &component) {

}

}
