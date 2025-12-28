#include <tui++/lookandfeel/MenuItemUI.h>

#include <tui++/MenuItem.h>

namespace tui::laf {
void MenuItemUI::install_ui(std::shared_ptr<Component> const &c) {
  this->menu_item = std::static_pointer_cast<MenuItem>(c).get();

  install_defaults();
  install_listeners();
}

void MenuItemUI::uninstall_ui(std::shared_ptr<Component> const &c) {
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
