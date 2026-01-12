#pragma once

#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/event/MenuDragMouseEvent.h>
#include <tui++/event/PropertyChangeEvent.h>

#include <functional>

namespace tui {
class Icon;
class MenuSelectionManager;
}

namespace tui::laf {

class LazyActionMap;

class MenuItemUI: public ComponentUI {
  using base = ComponentUI;

  MenuItem *menu_item;
  std::shared_ptr<Icon> check_icon;
  std::shared_ptr<Icon> arrow_icon;

protected:
  MouseOveredListener mouse_overed_listener = std::bind(&MenuItemUI::mouse_overed, this, std::placeholders::_1);
  MousePressedListener mouse_released_listener = std::bind(&MenuItemUI::mouse_released, this, std::placeholders::_1);
  MenuDragMouseOveredListener menu_drag_mouse_overed_listener = std::bind(&MenuItemUI::menu_drag_mouse_overed, this, std::placeholders::_1);
  MenuDragMouseDraggedListener menu_drag_mouse_dragged_listener = std::bind(&MenuItemUI::menu_drag_mouse_dragged, this, std::placeholders::_1);
  MenuDragMousePressedListener menu_drag_mouse_released_listener = std::bind(&MenuItemUI::menu_drag_mouse_released, this, std::placeholders::_1);
  PropertyChangeListener property_change_listener = std::bind(&MenuItemUI::property_changed, this, std::placeholders::_1);

public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override;
  virtual void uninstall_ui(std::shared_ptr<Component> const &c) override;

  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override;

  std::vector<std::shared_ptr<MenuElement>> get_path() const;

protected:
  virtual std::string const& get_property_prefix() const;

  virtual void install_defaults();
  virtual void install_listeners();
  virtual void install_keyboard_actions();

  virtual void uninstall_listeners();
  virtual void uninstall_keyboard_actions();

  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override;

protected:
  virtual void mouse_released(MousePressEvent &e);
  virtual void mouse_overed(MouseOverEvent &e);
  virtual void menu_drag_mouse_released(MenuDragMouseEvent<MousePressEvent> &e);
  virtual void menu_drag_mouse_overed(MenuDragMouseEvent<MouseOverEvent> &e);
  virtual void menu_drag_mouse_dragged(MenuDragMouseEvent<MouseDragEvent> &e);
  virtual void property_changed(PropertyChangeEvent &e);

  virtual void install_lazy_action_map();

  void do_click(std::shared_ptr<MenuSelectionManager> const &manager);

  static void load_action_map(LazyActionMap &map);

private:
  void update_accelerator_binding();
  void update_check_icon();

  bool do_not_close_on_mouse_click() const;
};

}

