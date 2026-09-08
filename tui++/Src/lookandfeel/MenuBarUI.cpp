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

  // As in Swing's BasicMenuBarUI: the bar is opaque and paints the theme's
  // menu background, so its menu items are readable on it (they use the theme's
  // menu text color). The colors are the "MenuBar.BackgroundColor" /
  // "MenuBar.ForegroundColor" theme keys, overridable per component.
  this->menu_bar->set_opaque(true);
  LookAndFeel::install_colors(this->menu_bar, "MenuBar.BackgroundColor", "MenuBar.ForegroundColor");
}

void MenuBarUI::install_listeners() {

}

void MenuBarUI::install_keyboard_actions() {

}

}
