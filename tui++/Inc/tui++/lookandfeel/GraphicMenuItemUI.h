#pragma once

#include <tui++/lookandfeel/MenuItemUI.h>

#include <tui++/Graphics.h>
#include <tui++/MenuItem.h>
#include <tui++/util/utf-8.h>

namespace tui::laf {

// Size of one terminal cell in the "graphic" (sixel) renderer, in pixels.
// Matches SixelScreen::CELL_WIDTH / SixelScreen::CELL_HEIGHT.
constexpr int GRAPHIC_CELL_WIDTH = 8;
constexpr int GRAPHIC_CELL_HEIGHT = 16;

// Pixel-level menu item UI: sizes and text positions are expressed in pixels
// instead of terminal cells.
class GraphicMenuItemUI: public MenuItemUI {
public:
  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    auto &&text = menu_item->get_text();
    auto width = text.empty() ? 0 : int(util::glyph_width(text));
    return Dimension { width * GRAPHIC_CELL_WIDTH + 2 * GRAPHIC_CELL_WIDTH, GRAPHIC_CELL_HEIGHT };
  }

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    g.draw_string(menu_item->get_text(), GRAPHIC_CELL_WIDTH, 0);
  }
};

}
