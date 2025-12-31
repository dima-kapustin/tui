#include <tui++/lookandfeel/MenuItemUI.h>
#include <tui++/lookandfeel/LazyActionMap.h>
#include <tui++/lookandfeel/UIAction.h>

#include <tui++/MenuItem.h>
#include <tui++/MenuSelectionManager.h>

namespace tui::laf {
constexpr std::string PROPERTY_PREFIX = "MenuItem";
constexpr std::string CLICK = "do_click";

class ClickAction: public UIAction {
public:
  ClickAction() :
      UIAction(CLICK) {
  }

  virtual void action_performed(ActionEvent &e) {
    MenuSelectionManager::single->clear_selected_path();
    std::static_pointer_cast<MenuItem>(e.source)->do_click();
  }
};

std::string const& MenuItemUI::get_property_prefix() const {
  return PROPERTY_PREFIX;
}

void MenuItemUI::install_ui(std::shared_ptr<Component> const &c) {
  this->menu_item = std::static_pointer_cast<MenuItem>(c).get();

  install_defaults();
  install_listeners();
  install_keyboard_actions();
}

void MenuItemUI::uninstall_ui(std::shared_ptr<Component> const &c) {
  uninstall_keyboard_actions();
  uninstall_listeners();
}

void MenuItemUI::install_defaults() {

}

void MenuItemUI::install_listeners() {
  this->menu_item->add_listener(this->mouse_overed_listener);
  this->menu_item->add_listener(this->mouse_pressed_listener);
  this->menu_item->add_listener(this->menu_drag_mouse_overed_listener);
  this->menu_item->add_listener(this->menu_drag_mouse_dragged_listener);
  this->menu_item->add_listener(this->menu_drag_mouse_pressed_listener);
  this->menu_item->add_property_change_listener(this->property_change_listener);
}

void MenuItemUI::uninstall_listeners() {
  this->menu_item->remove_property_change_listener(this->property_change_listener);
  this->menu_item->remove_listener(this->menu_drag_mouse_pressed_listener);
  this->menu_item->remove_listener(this->menu_drag_mouse_dragged_listener);
  this->menu_item->remove_listener(this->menu_drag_mouse_overed_listener);
  this->menu_item->remove_listener(this->mouse_pressed_listener);
  this->menu_item->remove_listener(this->mouse_overed_listener);
}

void MenuItemUI::install_keyboard_actions() {
  auto action_map = LookAndFeel::get<std::shared_ptr<ActionMap>>(get_property_prefix() + ".action-map");
  if (not action_map) {
    action_map = std::make_shared<LazyActionMap>(load_action_map);
  }
}

void MenuItemUI::load_action_map(LazyActionMap &map) {
  map.emplace(std::make_shared<ClickAction>());
}

void MenuItemUI::uninstall_keyboard_actions() {

}

void MenuItemUI::mouse_pressed(MousePressEvent &e) {

}

void MenuItemUI::mouse_overed(MouseOverEvent &e) {

}

void MenuItemUI::menu_drag_mouse_pressed(MenuDragMouseEvent<MousePressEvent> &e) {

}

void MenuItemUI::menu_drag_mouse_overed(MenuDragMouseEvent<MouseOverEvent> &e) {

}

void MenuItemUI::menu_drag_mouse_dragged(MenuDragMouseEvent<MouseDragEvent> &e) {

}

void MenuItemUI::property_changed(PropertyChangeEvent &e) {

}

}
