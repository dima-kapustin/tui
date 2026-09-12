#pragma once

#include <tui++/lookandfeel/MenuUI.h>
#include <tui++/lookandfeel/Translations.h>
#include <tui++/TextMetrics.h>

#include <tui++/Graphics.h>
#include <tui++/Insets.h>
#include <tui++/Menu.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/lookandfeel/basic/ToggleIndicator.h>
#include <tui++/MenuItem.h>
#include <tui++/Screen.h>
#include <tui++/Symbols.h>

namespace tui::laf {

// Pixel-level menu UI. Inherits MenuUI's behaviour but sizes and draws text
// in pixels, matching SixelMenuItemUI: the label is inset by the theme's
// "Menu.margin" and vertically centered, as in Swing's BasicLookAndFeel.
// A popup (sub)menu row indents its label behind the check/radio indicator
// column when the popup holds check/radio items; a top-level menu bar row
// never does (menu_check_column). A nested Menu row (a submenu) draws its
// arrow at the right edge.
class SixelMenuUI: public MenuUI {
public:
  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    auto &&text = displayed_text(*menu_item, menu_item->get_text());
    auto metrics = screen.get_text_metrics();
    auto width = text.empty() ? 0 : metrics->get_width(text);
    auto margin = LookAndFeel::get<Insets>("Menu.margin", Insets { 2, 2, 2, 2 });
    if (menu_check_column(menu_item.get())) {
      width += indicator_column_width(*metrics);
    }
    auto menu = dynamic_cast<Menu const*>(menu_item.get());
    if (menu and not menu->is_top_level_menu()) {
      width += metrics->get_char_width(Symbols::TRIANGLE_RIGHT_POINTING_BLACK.get_code()) + metrics->get_char_width(char32_t('M'));
    }
    return Dimension { width + margin.left + margin.right, metrics->get_line_height() };
  }

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override {
    auto menu_item = std::static_pointer_cast<const MenuItem>(c);
    auto margin = LookAndFeel::get<Insets>("Menu.margin", Insets { 2, 2, 2, 2 });

    // The hovered top-level menu is painted on the selection colors.
    if (menu_item->is_armed()) {
      g.set_background_color(get_selection_background(menu_item.get()));
      g.fill_rect(0, 0, menu_item->get_width(), menu_item->get_height());
      g.set_foreground_color(get_selection_foreground(menu_item.get()));
    }

    // The glyph is twice as tall as the font size (16x32 raster); center it
    // vertically in the area left inside the margin.
    auto glyph_height = 2 * g.get_font().get_size();
    auto y = margin.top + (menu_item->get_height() - margin.top - margin.bottom - glyph_height) / 2;

    // A submenu row of a popup with check/radio items indents behind the
    // indicator column like every other row; a top-level menu row does not.
    auto metrics = screen.get_text_metrics();
    auto label = displayed_text(*menu_item, menu_item->get_text());
    auto label_x = margin.left;
    auto menu = dynamic_cast<Menu const*>(menu_item.get());
    auto is_submenu = menu and not menu->is_top_level_menu();
    if (menu_check_column(menu_item.get())) {
      label_x += indicator_column_width(*metrics);
    }
    g.draw_string(label, label_x, y);

    // The submenu arrow at the right edge.
    if (is_submenu) {
      auto arrow = Symbols::TRIANGLE_RIGHT_POINTING_BLACK;
      auto arrow_w = metrics->get_char_width(arrow.get_code());
      auto x = menu_item->get_width() - margin.right - arrow_w;
      auto title_width = label.empty() ? 0 : metrics->get_width(label);
      if (x >= label_x + title_width) {
        g.draw_char(arrow, x, y);
      }
    }
  }
};

}
