#pragma once

#include <tui++/lookandfeel/text/TextLookAndFeel.h>

#include <tui++/terminal/graphic/GraphicTheme.h>

#include <tui++/lookandfeel/graphic/GraphicMenuItemUI.h>
#include <tui++/lookandfeel/graphic/GraphicMenuUI.h>

namespace tui::laf {

// Look-and-feel for the "graphic" (sixel) terminal. It reuses the text
// look-and-feel's delegates except for menu items and menus, whose sizes and
// text positions are expressed in pixels.
class GraphicLookAndFeel: public TextLookAndFeel {
public:
  GraphicLookAndFeel() {
    this->theme = std::make_shared<GraphicTheme>();
    init_theme(this->theme);
  }

  virtual std::shared_ptr<MenuItemUI> create_menu_item_ui(MenuItem *c) override {
    return std::make_shared<GraphicMenuItemUI>();
  }

  virtual std::shared_ptr<MenuUI> create_menu_ui(Menu *c) override {
    return std::make_shared<GraphicMenuUI>();
  }
};

}
