#include <tui++/lookandfeel/MenuItemUI.h>
#include <tui++/lookandfeel/LazyActionMap.h>
#include <tui++/lookandfeel/MenuLayout.h>

#include <tui++/lookandfeel/UIAction.h>
#include <tui++/lookandfeel/UIComponentInputMap.h>

#include <tui++/Menu.h>
#include <tui++/MenuItem.h>
#include <tui++/CheckBoxMenuItem.h>
#include <tui++/RadioButtonMenuItem.h>
#include <tui++/MenuSelectionManager.h>
#include <tui++/Graphics.h>

#include <cassert>

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
  this->menu_item->add_listener(MousePressEvent::MOUSE_RELEASED, this->mouse_released_listener);
  this->menu_item->add_listener(this->menu_drag_mouse_overed_listener);
  this->menu_item->add_listener(this->menu_drag_mouse_dragged_listener);
  this->menu_item->add_listener(this->menu_drag_mouse_released_listener);
  this->menu_item->add_property_change_listener(this->property_change_listener);
}

void MenuItemUI::uninstall_listeners() {
  this->menu_item->remove_property_change_listener(this->property_change_listener);
  this->menu_item->remove_listener(this->menu_drag_mouse_released_listener);
  this->menu_item->remove_listener(this->menu_drag_mouse_dragged_listener);
  this->menu_item->remove_listener(this->menu_drag_mouse_overed_listener);
  this->menu_item->remove_listener(MousePressEvent::MOUSE_RELEASED, this->mouse_released_listener);
  this->menu_item->remove_listener(this->mouse_overed_listener);
}

void MenuItemUI::install_keyboard_actions() {
  install_lazy_action_map();
  update_accelerator_binding();
}

void MenuItemUI::install_lazy_action_map() {
  auto property_name = get_property_prefix() + ".action-map";
  auto action_map = LookAndFeel::get<std::shared_ptr<ActionMap>>(property_name);
  if (not action_map) {
    action_map = std::make_shared<LazyActionMap>(load_action_map);
    LookAndFeel::put(property_name, action_map);
  }
  LookAndFeel::replace_action_map(this->menu_item, action_map);
}

void MenuItemUI::load_action_map(LazyActionMap &map) {
  map.emplace(std::make_shared<ClickAction>());
}

void MenuItemUI::update_accelerator_binding() {
  auto window_input_map = LookAndFeel::get_input_map(this->menu_item, Component::WHEN_IN_FOCUSED_WINDOW);
  if (window_input_map) {
    window_input_map->clear();
  }

  if (auto accelerator = this->menu_item->get_accelerator()) {
    if (not window_input_map) {
      window_input_map = std::make_shared<UIComponentInputMap>(this->menu_item);
      LookAndFeel::replace_input_map(this->menu_item, Component::WHEN_IN_FOCUSED_WINDOW, window_input_map);
    }

    window_input_map->emplace(accelerator.value(), CLICK);
  }
}

void MenuItemUI::uninstall_keyboard_actions() {

}

void MenuItemUI::mouse_released(MousePressEvent &e) {
  if (this->menu_item->is_enabled()) {

  }
}

void MenuItemUI::mouse_overed(MouseOverEvent &e) {

}

void MenuItemUI::menu_drag_mouse_released(MenuDragMouseEvent<MousePressEvent> &e) {

}

void MenuItemUI::menu_drag_mouse_overed(MenuDragMouseEvent<MouseOverEvent> &e) {

}

void MenuItemUI::menu_drag_mouse_dragged(MenuDragMouseEvent<MouseDragEvent> &e) {

}

void MenuItemUI::property_changed(PropertyChangeEvent &e) {

}

bool MenuItemUI::do_not_close_on_mouse_click() const {
  if (dynamic_cast<CheckBoxMenuItem*>(this->menu_item)) {
    constexpr auto property = "CheckBoxMenuItem.doNotCloseOnMouseClick";
    return LookAndFeel::get<bool>(this->menu_item, property);
  } else if (dynamic_cast<RadioButtonMenuItem*>(this->menu_item)) {
    constexpr auto property = "RadioButtonMenuItem.doNotCloseOnMouseClick";
    return LookAndFeel::get<bool>(this->menu_item, property);
  }
  return false;
}

void MenuItemUI::do_click(std::shared_ptr<MenuSelectionManager> const &manager) {

}

std::optional<Dimension> MenuItemUI::get_preferred_size(std::shared_ptr<const Component> const &c) const {
  assert(this->menu_item == std::dynamic_pointer_cast<const MenuItem>(c).get());
  if (auto menu = std::dynamic_pointer_cast<Menu>(this->menu_item->get_parent())) {
    if (auto menu_layout = std::dynamic_pointer_cast<MenuLayout>(menu->get_layout())) {
      return menu_layout->get_preferred_size(this->menu_item);
    }
  }
  auto &&text = this->menu_item->get_text();
  auto width = text.empty() ? 0 : int(text.length());
  return Dimension { width + 2, 1 };
}

void MenuItemUI::paint(Graphics &g, std::shared_ptr<const Component> const &c) const {
  assert(this->menu_item == std::dynamic_pointer_cast<const MenuItem>(c).get());
  g.draw_string(this->menu_item->get_text(), 1, 0);
}

}
