#pragma once

#include <tui++/lookandfeel/MenuItemUI.h>
#include <tui++/TextMetrics.h>

#include <tui++/Graphics.h>
#include <tui++/Insets.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/MenuItem.h>
#include <tui++/Screen.h>
#include <tui++/util/utf-8.h>

namespace tui::laf {

// Pixel-level menu item UI: sizes and text positions are expressed in pixels.
// The label is laid out the way Swing's BasicMenuItemUI does it: it is inset
// by the theme's "MenuItem.margin" (pixels on the graphic screen) and the
// text is vertically centered in the remaining area.
class SixelMenuItemUI: public MenuItemUI {
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

    // The hovered/selected item is painted on the selection colors; the fill
    // spans the whole row so the highlight reads as a solid band.
    if (menu_item->is_armed()) {
      g.set_background_color(get_selection_background(menu_item.get()));
      g.fill_rect(0, 0, menu_item->get_width(), menu_item->get_height());
      g.set_foreground_color(get_selection_foreground(menu_item.get()));
    }

    // The glyph is twice as tall as the font size (16x32 raster); center it
    // vertically in the area left inside the margin.
    auto glyph_height = 2 * g.get_font().get_size();
    auto y = margin.top + (menu_item->get_height() - margin.top - margin.bottom - glyph_height) / 2;
    g.draw_string(menu_item->get_text(), margin.left, y);

    // The mnemonic letter (the Alt+shortcut key, like "File" with mnemonic
    // 'F') is drawn bold with a single underline beneath the glyph, mirroring
    // the text screen's bold+underline mnemonic. The label was already drawn
    // above; the bold glyph is a superset of the plain one (each stroke is
    // mirrored one column to the right), so re-drawing the letter in bold
    // over it thickens it in place. The underline is a real pixel line in the
    // text color, spanning the glyph's cell at the bottom of the glyph box.
    auto mnemonic = menu_item->get_mnemonic().get_code();
    if (mnemonic != 0) {
      auto metrics = screen.get_text_metrics();
      auto const &text = menu_item->get_text();
      auto wanted = mnemonic >= 'A' and mnemonic <= 'Z' ? mnemonic - 'A' + 'a' : mnemonic;
      auto x = margin.left;
      auto index = std::size_t { 0 };
      while (index < text.size()) {
        auto code = char32_t { };
        auto len = util::mb_to_c32(text.data() + index, text.size() - index, &code);
        if (len <= 0) {
          index += 1;
          continue;
        }
        auto folded = code >= 'A' and code <= 'Z' ? code - 'A' + 'a' : code;
        if (folded == wanted) {
          auto font = g.get_font();
          auto style = font.get_style();
          font.set_style(style | Font::BOLD);
          g.set_font(font);
          g.draw_char(Char(code), x, y);
          font.set_style(style);
          g.set_font(font);
          g.draw_hline(x, y + glyph_height - 1, metrics->get_char_width(code));
          break;
        }
        x += metrics->get_char_width(code);
        index += std::size_t(len);
      }
    }
  }
};

}
