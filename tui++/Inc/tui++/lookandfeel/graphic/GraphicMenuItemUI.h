#pragma once

#include <tui++/lookandfeel/MenuItemUI.h>
#include <tui++/TextMetrics.h>

#include <tui++/Graphics.h>
#include <tui++/Insets.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/MenuItem.h>
#include <tui++/Screen.h>
#include <tui++/terminal/graphic/Font8x8.h>

namespace tui::laf {

// Pixel-level menu item UI: sizes and text positions are expressed in pixels.
// The label is laid out the way Swing's BasicMenuItemUI does it: it is inset
// by the theme's "MenuItem.margin" (pixels on the graphic screen) and the
// text is vertically centered in the remaining area.
class GraphicMenuItemUI: public MenuItemUI {
public:
  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    auto &&text = menu_item->get_text();
    auto metrics = screen.get_text_metrics();
    auto width = text.empty() ? 0 : metrics->get_width(text);
    auto margin = LookAndFeel::get<Insets>("MenuItem.margin", Insets { 2, 2, 2, 2 });
    return Dimension { width + margin.left + margin.right, metrics->get_line_height() };
  }

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    auto margin = LookAndFeel::get<Insets>("MenuItem.margin", Insets { 2, 2, 2, 2 });
    auto y = margin.top + (menu_item->get_height() - margin.top - margin.bottom - detail::FONT_HEIGHT) / 2;
    g.draw_string(menu_item->get_text(), margin.left, y);
  }
};

}
