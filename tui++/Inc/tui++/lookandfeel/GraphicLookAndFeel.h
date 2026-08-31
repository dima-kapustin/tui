#pragma once

#include <tui++/lookandfeel/TextLookAndFeel.h>

#include <tui++/lookandfeel/GraphicMenuItemUI.h>
#include <tui++/lookandfeel/GraphicMenuUI.h>

namespace tui::laf {

// Look-and-feel for the "graphic" (sixel) terminal. It reuses the text
// look-and-feel's delegates except for menu items and menus, whose sizes and
// text positions are expressed in pixels.
class GraphicLookAndFeel: public TextLookAndFeel {
  virtual std::shared_ptr<MenuItemUI> create_menu_item_ui(MenuItem *c) override {
    return std::make_shared<GraphicMenuItemUI>();
  }

  virtual std::shared_ptr<MenuUI> create_menu_ui(Menu *c) override {
    return std::make_shared<GraphicMenuUI>();
  }
};

}
