#include <tui++/lookandfeel/SwitchUI.h>

#include <tui++/AbstractButton.h>
#include <tui++/Color.h>
#include <tui++/Component.h>
#include <tui++/Graphics.h>
#include <tui++/Symbols.h>
#include <tui++/TextMetrics.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/lookandfeel/basic/ToggleIndicator.h>

#include <algorithm>

namespace tui::laf {

namespace {

// The cells the track spans between its two ends, and the colors of the track
// in each state and of the thumb (a program can override them per switch with
// client properties of the same names, the way every "LookAndFeel::get(component,
// ...)" lookup works).
constexpr auto TRACK_WIDTH_KEY = "Switch.TrackWidth";
constexpr auto TRACK_COLOR_KEY = "Switch.TrackColor";
constexpr auto TRACK_SELECTED_COLOR_KEY = "Switch.TrackSelectedColor";
constexpr auto THUMB_COLOR_KEY = "Switch.ThumbColor";

constexpr int TRACK_WIDTH_DEFAULT = 3;
constexpr auto TRACK_COLOR_DEFAULT = Color { 0xA0, 0xA0, 0xA0 };
constexpr auto TRACK_SELECTED_COLOR_DEFAULT = Color { 0x00, 0x00, 0x80 };
constexpr auto THUMB_COLOR_DEFAULT = Color { 0xFF, 0xFF, 0xFF };

// The screen's unit of one cell: one column on the text screen, the glyph
// width in pixels on the graphic one, so the track spans the same number of
// cells (and the same look) on both backends.
int unit_width(TextMetrics const &metrics) {
  return metrics.get_char_width(char32_t('M'));
}

// The cells between the two rounded ends (at least one, so the thumb always
// has a cell of its own).
int track_cells(Component const &c) {
  return std::max(1, LookAndFeel::get<int>(&c, TRACK_WIDTH_KEY, TRACK_WIDTH_DEFAULT));
}

}

void SwitchUI::paint(Graphics &g, std::shared_ptr<const Component> const &c) const {
  // No "pressed" face fill (see ButtonUI::paint): the track shows the state.
  paint_content(g, c);
}

int SwitchUI::leading_width(TextMetrics const &metrics, Component const &c) const {
  // The two ends, the track between them, and the gap to the label.
  return (track_cells(c) + 2 + INDICATOR_GAP_COLUMNS) * unit_width(metrics);
}

int SwitchUI::paint_leading(Graphics &g, TextMetrics const &metrics, Component const &c, int x, int y) const {
  auto const &button = static_cast<AbstractButton const &>(c);
  auto unit = unit_width(metrics);
  auto cells = track_cells(c);
  auto width = (cells + 2) * unit;

  // The cells under the ends and between them carry the track's color: the
  // selected state takes its own, so on and off read at a glance.
  auto track_color = LookAndFeel::get<Color>(&c, button.is_selected() ? TRACK_SELECTED_COLOR_KEY : TRACK_COLOR_KEY, button.is_selected() ? TRACK_SELECTED_COLOR_DEFAULT : TRACK_COLOR_DEFAULT);
  g.set_background_color(track_color);
  g.fill_rect(x, y, width, metrics.get_line_height());

  // A switch with the focus underlines its track: the focus marker of a widget
  // that has no label of its own (a labelled one also gets its label
  // underlined by the shared label painting).
  auto focus = button.is_focus_owner() and button.is_focus_painted();
  auto attributes = focus ? std::optional<Attributes> { Attribute::UNDERLINE } : std::nullopt;

  // The rounded ends...
  g.set_foreground_color(button.get_foreground_color().value_or(BLACK_COLOR));
  g.draw_char(Symbols::SWITCH_TRACK_LEFT, x, y, attributes);
  g.draw_char(Symbols::SWITCH_TRACK_RIGHT, x + width - unit, y, attributes);

  // ... and the thumb, in the first cell of the off state and the last of the
  // on one (its travel is what shows the switch's state).
  auto thumb_cell = button.is_selected() ? cells - 1 : 0;
  g.set_foreground_color(LookAndFeel::get<Color>(&c, THUMB_COLOR_KEY, THUMB_COLOR_DEFAULT));
  g.draw_char(Symbols::SWITCH_THUMB, x + unit * (1 + thumb_cell), y, attributes);

  return leading_width(metrics, c);
}

}
