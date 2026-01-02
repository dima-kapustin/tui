#include <tui++/lookandfeel/MenuLayout.h>

#include <tui++/PopupMenu.h>

namespace tui::laf {

std::optional<Dimension> MenuLayout::get_preferred_layout_size(const std::shared_ptr<const Component> &target) {
  if (auto popup_menu = std::dynamic_pointer_cast<const PopupMenu>(target)) {
    if (popup_menu->get_component_count() == 0) {
      return Dimension::zero();
    }
  }

  // Force recalculation of preferred sizes
  invalidate_layout(target);
  return base::get_preferred_layout_size(target);
}

Dimension MenuLayout::get_preferred_size(MenuItem const *menu_item) {
  return {};
}

}
