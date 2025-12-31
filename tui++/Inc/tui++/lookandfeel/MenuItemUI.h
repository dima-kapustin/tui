#pragma once

#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/event/MenuDragMouseEvent.h>
#include <tui++/event/PropertyChangeEvent.h>

#include <functional>

namespace tui::laf {

class LazyActionMap;

class MenuItemUI: public ComponentUI {
  MenuItem * menu_item;

protected:
  MouseOveredListener mouse_overed_listener = std::bind(mouse_overed, this, std::placeholders::_1);
  MousePressedListener mouse_pressed_listener = std::bind(mouse_pressed, this, std::placeholders::_1);
  MenuDragMouseOveredListener menu_drag_mouse_overed_listener = std::bind(menu_drag_mouse_overed, this, std::placeholders::_1);
  MenuDragMouseDraggedListener menu_drag_mouse_dragged_listener = std::bind(menu_drag_mouse_dragged, this, std::placeholders::_1);
  MenuDragMousePressedListener menu_drag_mouse_pressed_listener = std::bind(menu_drag_mouse_pressed, this, std::placeholders::_1);
  PropertyChangeListener property_change_listener = std::bind(property_changed, this, std::placeholders::_1);

public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override;
  virtual void uninstall_ui(std::shared_ptr<Component> const &c) override;

protected:
  virtual std::string const& get_property_prefix() const;

  virtual void install_defaults();
  virtual void install_listeners();
  virtual void install_keyboard_actions();

  virtual void uninstall_listeners();
  virtual void uninstall_keyboard_actions();

private:
  virtual void mouse_pressed(MousePressEvent &e);
  virtual void mouse_overed(MouseOverEvent &e);
  virtual void menu_drag_mouse_pressed(MenuDragMouseEvent<MousePressEvent> &e);
  virtual void menu_drag_mouse_overed(MenuDragMouseEvent<MouseOverEvent> &e);
  virtual void menu_drag_mouse_dragged(MenuDragMouseEvent<MouseDragEvent> &e);
  virtual void property_changed(PropertyChangeEvent &e);

  static void load_action_map(LazyActionMap &map);
};

}

