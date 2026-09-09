#include <tui++/lookandfeel/PopupMenuSeparatorUI.h>

#include <tui++/Graphics.h>
#include <tui++/Insets.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/PopupMenu.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>

#include <cstdint>

namespace tui::laf {

std::optional<Dimension> PopupMenuSeparatorUI::get_preferred_size(std::shared_ptr<const Component> const &c) const {
  // A separator row is as tall as a menu item row (one cell on the text
  // screen, the pixel line height on the graphic screen) and never widens the
  // popup: the horizontal cross-axis distribution stretches it to the popup's
  // full width, so its preferred width stays zero.
  return Dimension { 0, screen.get_text_metrics()->get_line_height() };
}

void PopupMenuSeparatorUI::paint(Graphics &g, std::shared_ptr<const Component> const &c) const {
  // The rule's color is the midpoint between the popup menu's background and
  // text (Swing's BasicPopupMenuSeparatorUI draws "Separator.foreground", a
  // gray between the two, over the menu background). Blending the two colors
  // keeps the separator visible on light and dark themes alike, and follows a
  // popup whose colors the application overrode.
  auto background = Color { 0xC0, 0xC0, 0xC0 };
  auto foreground = Color { 0, 0, 0 };
  if (auto parent = c->get_parent()) {
    if (auto popup_menu = std::dynamic_pointer_cast<PopupMenu>(parent)) {
      if (auto color = popup_menu->get_background_color()) {
        background = color.value();
      }
      if (auto color = popup_menu->get_foreground_color()) {
        foreground = color.value();
      }
    }
  }
  auto line_color = Color { //
    std::uint8_t((int(background.red()) + int(foreground.red())) / 2), //
    std::uint8_t((int(background.green()) + int(foreground.green())) / 2), //
    std::uint8_t((int(background.blue()) + int(foreground.blue())) / 2) //
  };

  // The cells the rule does not cover keep the popup's background; set it
  // explicitly so a repaint of just the separator row matches the fill of a
  // full popup repaint.
  g.set_background_color(background);
  g.set_foreground_color(line_color);

  // Inset the rule by the row's margins (aligned with the item text column:
  // "MenuItem.margin" is one cell left and right on the text screen).
  auto margin = LookAndFeel::get<Insets>("PopupMenuSeparator.margin", Insets { 0, 1, 0, 1 });
  if (c->get_width() <= margin.left + margin.right or c->get_height() <= 0) {
    return;
  }
  // The graphics draw the line around the requested row (the text screen
  // centers its box-drawing glyph in the cell, the graphic screen centers
  // its stroke), so the middle of the row reads as a centered rule.
  g.draw_hline(margin.left, c->get_height() / 2, c->get_width() - margin.left - margin.right);
}

}
