#pragma once

#include <tui++/ToggleButton.h>

namespace tui {
namespace laf {
class SwitchUI;
}

// The horizontal two-state switch: a track with two rounded ends and a round
// thumb that slides from one end to the other -- the character-cell stand-in
// for the platform switch (Swing has no switch widget of its own; this is a
// toggle button's model and behaviour drawn as one).
//
// Like CheckBox and RadioButton it is a toggleable button: a click or Space
// flips it, a ButtonGroup can make a set of switches exclusive, an Action can
// drive it, and its optional label trails the track. The track and the thumb
// are painted by the look-and-feel's SwitchUI from the theme's "Switch.*"
// values (the track's color in both states, the thumb, and how many cells the
// track spans), so the switch is styled like every other widget.
class Switch: public ToggleButton {
  using base = ToggleButton;

protected:
  Switch(std::string const &text = "") :
      Switch(text, Char { }) {
  }

  Switch(std::string const &text, Char const &mnemonic) :
      base(text, mnemonic) {
  }

  // The switch's own look, the way Swing's BasicCheckBoxUI gives its kind one:
  // no raised bezel around the track, and the label trails it (the horizontal
  // alignment only matters when the switch is wider than its content).
  virtual void init() override {
    base::init();
    set_border_painted(false);
    set_horizontal_alignment(HorizontalAlignment::LEADING);
    set_horizontal_text_position(HorizontalTextPosition::TRAILING);
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

public:
  std::shared_ptr<laf::SwitchUI> get_ui() const;

protected:
  virtual std::shared_ptr<laf::ComponentUI> create_ui() override;
};

}
