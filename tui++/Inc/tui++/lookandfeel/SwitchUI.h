#pragma once

#include <tui++/lookandfeel/ToggleButtonUI.h>

namespace tui::laf {

// The switch's UI delegate: the toggle button family's shared behavior -- the
// model, the mouse and the keyboard, the label painting -- with the switch's
// horizontal track in place of the check/radio indicator (see
// ButtonUI::paint_leading).
//
// The track is one row (one glyph line on the pixel screen): the two rounded
// ends, the cells between them filled with the track's color, and the round
// thumb. The on and off states differ in the track's color (and the end the
// thumb sits at), so the state reads at a glance; the track's width is a theme
// value, and its room plus the label is the switch's preferred size.
class SwitchUI: public ToggleButtonUI {
  using base = ToggleButtonUI;

protected:
  // The switch paints no "pressed" face fill: the track itself carries the
  // state (a press flips the model on release, and the track follows).
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override;

  // The leading visual is the track: the two ends and the cells between them,
  // plus the gap to the label.
  virtual int leading_width(TextMetrics const &metrics, Component const &c) const override;

  // Draws the track at (x, y) and returns its width: the cells under the ends
  // and between them take the track's color (a different one for the on
  // state), the ends are glyphs and the thumb sits in the first or the last
  // cell between them.
  virtual int paint_leading(Graphics &g, TextMetrics const &metrics, Component const &c, int x, int y) const override;
};

}
