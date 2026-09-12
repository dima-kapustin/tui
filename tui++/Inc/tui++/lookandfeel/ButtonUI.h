#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

#include <tui++/Rectangle.h>
#include <tui++/TextMetrics.h>

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

  // The content painting: the leading visual, the icon and the label, laid out
  // and aligned inside the content area (inside the border and the margin).
  // The face fill and the border are the component's own (see
  // ComponentUI::update and Component::paint_border). A subclass whose kind
  // paints no "pressed" face (the switch) overrides paint and calls this.
  virtual void paint_content(Graphics &g, std::shared_ptr<const Component> const &c) const;

  // The width of the whole content line: the leading visual, the icon, the
  // icon-text gap and the label. The preferred size and the paint both measure
  // through it, so a laid-out button always has room for what it paints.
  virtual int content_width(TextMetrics const &metrics, Component const &c) const;

  // The leading visual: the check/radio indicator of the toggle button family,
  // nothing for the other kinds. A subclass replaces it with a visual of its
  // own (the switch's track). `leading_width` measures it, `paint_leading`
  // draws it at (x, y) -- the content area's top-left -- and returns the space
  // it used. A leading visual may paint in a palette of its own; the caller
  // sets the icon's and the label's colors after it, so the label keeps the
  // button's own.
  virtual int leading_width(TextMetrics const &metrics, Component const &c) const;
  virtual int paint_leading(Graphics &g, TextMetrics const &metrics, Component const &c, int x, int y) const;

  // The content area of `c`: the component inside its border and its margin,
  // or an empty rectangle when there is no room for content.
  Rectangle get_content_area(std::shared_ptr<const Component> const &c) const;

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
