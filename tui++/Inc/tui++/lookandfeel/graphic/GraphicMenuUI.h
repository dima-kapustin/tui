#pragma once

#include <tui++/lookandfeel/MenuUI.h>
#include <tui++/TextMetrics.h>

#include <tui++/Graphics.h>
#include <tui++/MenuItem.h>
#include <tui++/Screen.h>

namespace tui::laf {

// Pixel-level menu UI. Inherits MenuUI's behaviour but sizes and draws text
// in pixels, matching GraphicMenuItemUI.
class GraphicMenuUI: public MenuUI {
public:
  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    auto &&text = menu_item->get_text();
    auto metrics = screen.get_text_metrics();
    auto width = text.empty() ? 0 : metrics->get_width(text);
    return Dimension { width + 2 * GRAPHIC_CELL_WIDTH, metrics->get_line_height() };
  }

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    g.draw_string(menu_item->get_text(), GRAPHIC_CELL_WIDTH, 0);
  }
};

}
