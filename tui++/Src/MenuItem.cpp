#include <tui++/MenuItem.h>

namespace tui {
void MenuItem::process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {
}

void MenuItem::process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {
}

void MenuItem::menu_selection_changed(bool is_included) {
}

std::vector<std::shared_ptr<MenuElement>> MenuItem::get_sub_elements() {
  return {};
}

std::shared_ptr<Component> MenuItem::get_component() {
  return shared_from_this();
}

}
