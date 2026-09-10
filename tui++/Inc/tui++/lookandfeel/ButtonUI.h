#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

#include <tui++/event/MouseEvent.h>

#include <functional>

namespace tui {
class AbstractButton;
}

namespace tui::laf {

class LazyActionMap;

// Swing's BasicButtonUI: installs the button's theme defaults (colors and
// border) and the listeners that give it its mouse and keyboard behavior, and
// paints its content -- the check/radio indicator of the toggle button family,
// the optional icon and the label with its mnemonic -- plus the pressed/selected
// state, and reports the preferred size. Every button kind (Button,
// ToggleButton, CheckBox, RadioButton) shares this delegate; ToggleButtonUI
// subclasses it so a look-and-feel can hand a distinct UI class to the
// toggleable kinds.
class ButtonUI: public ComponentUI {
  AbstractButton *button = nullptr;

protected:
  // The mouse behavior of Swing's BasicButtonListener: a left press takes the
  // focus and arms and presses the model (the "down" look), the release drops
  // the pressed state -- which fires the action while the button is still
  // armed -- and a pointer that leaves a held button disarms it, so the
  // release over something else does not fire.
  MousePressedListener mouse_pressed_listener = std::bind(&ButtonUI::mouse_pressed, this, std::placeholders::_1);
  MousePressedListener mouse_released_listener = std::bind(&ButtonUI::mouse_released, this, std::placeholders::_1);
  MouseOveredListener mouse_overed_listener = std::bind(&ButtonUI::mouse_overed, this, std::placeholders::_1);

public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override;

  virtual void uninstall_ui(std::shared_ptr<Component> const &c) override;

  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override;

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override;

  virtual void install_listeners();
  virtual void uninstall_listeners();

  // The button's keyboard behavior, installed from the theme's shared
  // "Button.ActionMap"/"Button.FocusInputMap" resources (Swing's
  // BasicButtonUI.installKeyboardActions and the "Button.actionMap" /
  // "Button.focusInputMap" UI defaults). Every kind of button takes the same
  // gestures, so the whole family shares the two maps.
  virtual void install_keyboard_actions();
  virtual void uninstall_keyboard_actions();

  virtual void install_lazy_action_map();
  virtual void install_focus_input_map();

  static void load_action_map(LazyActionMap &map);

  virtual void mouse_pressed(MousePressEvent &e);
  virtual void mouse_released(MousePressEvent &e);
  virtual void mouse_overed(MouseOverEvent &e);
};

}
