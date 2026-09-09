#pragma once

#include <tui++/Char.h>
#include <tui++/Graphics.h>
#include <tui++/Symbols.h>
#include <tui++/TextMetrics.h>

#include <algorithm>

namespace tui {
class Graphics;
class MenuItem;
}

namespace tui::laf {

// The check/radio indicator shared by the toggle button family (CheckBox,
// RadioButton) and the check/radio menu items. The drawing is expressed in
// glyph columns of the screen's own unit: one terminal cell on the text
// screen, one glyph column (the font size) in pixels on the graphic screen,
// so the same helpers paint correctly on both backends.
//
// A check box shows an empty box (☐) or a box with a check (☒); a radio
// button shows a hollow circle (○) or a filled one (●) -- the text-terminal
// stand-ins for the platform checkbox/radio glyphs.
enum class IndicatorKind {
  NONE,
  CHECKBOX,
  RADIO,
};

// The indicator a check/radio menu item paints (CHECKBOX for a
// CheckBoxMenuItem, RADIO for a RadioButtonMenuItem, NONE otherwise).
IndicatorKind menu_item_indicator(MenuItem const *item);

// Whether the rows of `item`'s popup reserve the indicator column in front
// of the label: every row of a popup that contains at least one check/radio
// item is indented so their labels (and the labels of the plain rows) line
// up, the way Swing's BasicMenuItemUI gives every row of such a popup a
// check icon column. Top-level menu bar menus never reserve it.
bool menu_check_column(MenuItem const *item);

// A column of whitespace between the indicator and the label, mirroring
// Swing's check icon text gap.
constexpr int INDICATOR_GAP_COLUMNS = 1;

// The glyph of the indicator's state: the empty/checked box, the
// hollow/filled circle.
inline Char indicator_glyph(IndicatorKind kind, bool is_selected) {
  switch (kind) {
  case IndicatorKind::CHECKBOX:
    return is_selected ? Symbols::CHECK_BOX_CHECKED : Symbols::CHECK_BOX;
  case IndicatorKind::RADIO:
    return is_selected ? Symbols::RADIO_BUTTON_CHECKED : Symbols::RADIO_BUTTON;
  case IndicatorKind::NONE:
    break;
  }
  return Char(' ');
}

// The width of the indicator glyphs of `kind` (either state), in the
// screen's units. Measured through the text metrics so a font that renders
// the glyph wide reserves the room it really needs.
inline int indicator_glyph_width(TextMetrics const &metrics, IndicatorKind kind) {
  if (kind == IndicatorKind::NONE) {
    return 0;
  }
  return std::max(metrics.get_char_width(indicator_glyph(kind, false).get_code()), //
                  metrics.get_char_width(indicator_glyph(kind, true).get_code()));
}

// The width reserved for an indicator plus the gap to the label, in the
// screen's units. All indicator kinds share the widest glyph, so mixed
// check/radio popups keep every row's label column aligned.
inline int indicator_column_width(TextMetrics const &metrics) {
  auto widest = std::max({ indicator_glyph_width(metrics, IndicatorKind::CHECKBOX), //
                           indicator_glyph_width(metrics, IndicatorKind::RADIO) });
  return widest + INDICATOR_GAP_COLUMNS * metrics.get_char_width(char32_t('M'));
}

// Draws the indicator for the check/radio button kinds at (x, y), using the
// graphics context's current colors. The indicator's column (see
// indicator_column_width) starts at x and the glyph is centered in it.
inline void paint_indicator(Graphics &g, TextMetrics const &metrics, IndicatorKind kind, int x, int y, bool is_selected) {
  if (kind == IndicatorKind::NONE) {
    return;
  }
  auto column = indicator_column_width(metrics);
  auto glyph = indicator_glyph(kind, is_selected);
  auto glyph_width = metrics.get_char_width(glyph.get_code());
  g.draw_char(glyph, x + (column - glyph_width) / 2, y);
}

}
