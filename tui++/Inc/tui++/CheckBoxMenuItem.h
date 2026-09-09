#pragma once

#include <tui++/MenuItem.h>

namespace tui {

// Swing's JCheckBoxMenuItem: a menu item with a two-state check box
// indicator. Its model is a ToggleButtonModel, so picking the item toggles
// its selected state (the way a regular JMenuItem fires once, this item
// fires on every pick -- toggled on, toggled off). The popup menu stays
// closed after a pick unless the theme property
// "CheckBoxMenuItem.DoNotCloseOnMouseClick" is set.
class CheckBoxMenuItem: public MenuItem {
  using base = MenuItem;

protected:
  CheckBoxMenuItem(std::string const &text = "") :
      CheckBoxMenuItem(text, Char { }) {
  }

  CheckBoxMenuItem(std::string const &text, Char const &mnemonic) :
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
