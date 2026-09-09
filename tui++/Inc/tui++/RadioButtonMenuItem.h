#pragma once

#include <tui++/MenuItem.h>

namespace tui {

// Swing's JRadioButtonMenuItem: a menu item with a round radio indicator.
// Radio items are meant to be added to a ButtonGroup, which turns a set of
// them into a single exclusive choice -- picking one unchecks the current
// selection of the group.
class RadioButtonMenuItem: public MenuItem {
  using base = MenuItem;

protected:
  RadioButtonMenuItem(std::string const &text = "") :
      RadioButtonMenuItem(text, Char { }) {
  }

  RadioButtonMenuItem(std::string const &text, Char const &mnemonic) :
      base(text, mnemonic) {
  }

  // The toggle model replaces the plain ButtonModel the MenuItem constructor
  // installed; done in init() so the item is owned by a shared pointer (the
  // model swap repaints).
  virtual void init() override;

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

}
