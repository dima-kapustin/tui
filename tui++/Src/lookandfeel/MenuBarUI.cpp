#include <tui++/lookandfeel/MenuBarUI.h>
#include <tui++/lookandfeel/MenuLayout.h>

#include <tui++/MenuBar.h>

namespace tui::laf {

void MenuBarUI::install_ui(std::shared_ptr<Component> const &c) {
  this->menu_bar = static_cast<MenuBar*>(c.get());
  install_defaults();
}

void MenuBarUI::install_defaults() {
  if (not this->menu_bar->get_layout()) {
    this->menu_bar->set_layout(std::make_shared<MenuLayout>(this->menu_bar, MenuLayout::LINE));
  }

  this->menu_bar->set_opaque(true);
}

void MenuBarUI::install_listeners() {

}

void MenuBarUI::install_keyboard_actions() {

}

}
