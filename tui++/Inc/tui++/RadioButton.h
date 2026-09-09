#pragma once

#include <tui++/ToggleButton.h>

namespace tui {

// Swing's JRadioButton: a two-state toggle button rendered with a round
// indicator in front of the label. Radio buttons are meant to be added to a
// ButtonGroup, whose single-selection model makes the group behave like a
// single choice (checking one member unchecks the current one).
class RadioButton: public ToggleButton {
  using base = ToggleButton;

protected:
  RadioButton(std::string const &text = "") :
      RadioButton(text, Char { }) {
  }

  RadioButton(std::string const &text, Char const &mnemonic) :
      base(text, mnemonic) {
  }

  // Swing's BasicRadioButtonUI defaults: no bezel, label trails the round
  // indicator (see CheckBox::init for why these live in init()).
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
