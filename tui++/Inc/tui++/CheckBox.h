#pragma once

#include <tui++/ToggleButton.h>

namespace tui {

// Swing's JCheckBox: a two-state toggle button rendered with a square check
// box indicator in front of the label. Like every toggleable button it can
// take part in a ButtonGroup; unlike a radio button Swing lets a grouped
// check box be cleared again (the group only arbitrates new selections).
class CheckBox: public ToggleButton {
  using base = ToggleButton;

protected:
  CheckBox(std::string const &text = "") :
      CheckBox(text, Char { }) {
  }

  CheckBox(std::string const &text, Char const &mnemonic) :
      base(text, mnemonic) {
  }

  // The look of Swing's BasicCheckBoxUI: no raised bezel around the label,
  // and the label trails the indicator (the horizontal alignment only
  // matters when the box is wider than the label). Applied in init() -- the
  // component is owned by a shared pointer by then, which the repainting
  // setters need.
  virtual void init() override {
    base::init();
    set_border_painted(false);
    set_horizontal_alignment(HorizontalAlignment::LEADING);
    set_horizontal_text_position(HorizontalTextPosition::TRAILING);
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

}
