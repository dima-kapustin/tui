#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

namespace tui {
class AbstractButton;
}

namespace tui::laf {

// Swing's BasicButtonUI: installs the button's theme defaults (colors and
// border) and paints its content -- the check/radio indicator of the toggle
// button family, the optional icon and the label with its mnemonic -- plus
// the pressed/selected state, and reports the preferred size. Every button
// kind (Button, ToggleButton, CheckBox, RadioButton) shares this delegate;
// ToggleButtonUI subclasses it so a look-and-feel can hand a distinct UI
// class to the toggleable kinds.
class ButtonUI: public ComponentUI {
public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override;

  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override;

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override;
};

}
