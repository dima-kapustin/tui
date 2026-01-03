#include <tui++/lookandfeel/MenuLayout.h>

#include <tui++/Menu.h>
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

bool MenuLayout::use_check_and_arrow(MenuItem const *menu_item) {
  if (auto menu = dynamic_cast<Menu const*>(menu_item)) {
    return not menu->is_top_level_menu();
  }
  return true;
}

}
