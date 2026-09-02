#pragma once

#include <tui++/lookandfeel/text/TextLookAndFeel.h>

#include <tui++/terminal/sixel/SixelTheme.h>

#include <tui++/lookandfeel/sixel/SixelMenuItemUI.h>
#include <tui++/lookandfeel/sixel/SixelMenuUI.h>

namespace tui::laf {

// Look-and-feel for the "sixel" (sixel) terminal. It reuses the text
// look-and-feel's delegates except for menu items and menus, whose sizes and
// text positions are expressed in pixels.
class SixelLookAndFeel: public TextLookAndFeel {
public:
  SixelLookAndFeel() {
    this->theme = std::make_shared<SixelTheme>();
    init_theme(this->theme);
  }

  virtual std::shared_ptr<MenuItemUI> create_menu_item_ui(MenuItem *c) override {
    return std::make_shared<SixelMenuItemUI>();
  }

  virtual std::shared_ptr<MenuUI> create_menu_ui(Menu *c) override {
    return std::make_shared<SixelMenuUI>();
  }
};

}
