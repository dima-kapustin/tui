#include <tui++/MenuItem.h>

#include <tui++/event/MenuKeyEvent.h>
#include <tui++/event/MenuDragMouseEvent.h>

#include <tui++/lookandfeel/MenuItemUI.h>

namespace tui {
std::shared_ptr<laf::MenuItemUI> MenuItem::get_ui() const {
  return std::static_pointer_cast<laf::MenuItemUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> MenuItem::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

MenuItem::MenuItem(std::string const &text) {

}

void MenuItem::process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {
  switch (e.id) {
  case MousePressEvent::MOUSE_RELEASED: {
    auto menu_event = MenuDragMouseEvent<MousePressEvent> { static_cast<MousePressEvent&>(e), path, manager };
    process_event(menu_event);
    break;
  }
  case MouseOverEvent::MOUSE_ENTERED:
  case MouseOverEvent::MOUSE_EXITED: {
    auto menu_event = MenuDragMouseEvent<MouseOverEvent> { static_cast<MouseOverEvent&>(e), path, manager };
    process_event(menu_event);
    break;
  }
  case MouseDragEvent::MOUSE_DRAGGED: {
    auto menu_event = MenuDragMouseEvent<MouseDragEvent> { static_cast<MouseDragEvent&>(e), path, manager };
    process_event(menu_event);
    break;
  }
  }
}

void MenuItem::process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {
  auto menu_event = MenuKeyEvent { e, path, manager };
  process_event(menu_event);
  if (menu_event.consumed) {
    e.consume();
  }
}

void MenuItem::process_event(MenuKeyEvent &e) {
  base::process_event(e);
}

void MenuItem::process_event(MenuDragMouseEvent<MousePressEvent> &e) {
  if (this->is_mouse_dragged) {
    base::process_event(e);
  }
}

void MenuItem::process_event(MenuDragMouseEvent<MouseDragEvent> &e) {
  this->is_mouse_dragged = true;
  base::process_event(e);
}

void MenuItem::process_event(MenuDragMouseEvent<MouseOverEvent> &e) {
  this->is_mouse_dragged = false;
  base::process_event(e);
}

void MenuItem::menu_selection_changed(bool is_included) {
  set_armed(is_included);
}

std::vector<std::shared_ptr<MenuElement>> MenuItem::get_sub_elements() {
  return {};
}

std::shared_ptr<Component> MenuItem::get_component() {
  return shared_from_this();
}

void MenuItem::init() {
  set_focusable(false);
  set_focus_painted(false);
  set_border_painted(false);
  set_horizontal_text_position(HorizontalTextPosition::TRAILING);
  set_horizontal_alignment(HorizontalAlignment::LEADING);
  update_ui();
}

void MenuItem::process_event(FocusEvent &e) {
  if (e.id == FocusEvent::FOCUS_LOST) {
    if (is_focus_painted()) {
      repaint();
    }
  }
  base::process_event(e);
}

void MenuItem::set_enabled(bool value) {
  if (not value /*and not UIManager.getBoolean("MenuItem.disabledAreNavigable")*/) {
    set_armed(false);
  }
  base::set_enabled(value);
}

void MenuItem::set_model(std::shared_ptr<ButtonModel> const &model) {
  base::set_model(model);
  model->make_menu_item();
}

void MenuItem::configure_properties_from_action() {
  base::configure_properties_from_action();
  set_accelerator(this->action->get_accelerator());
}

void MenuItem::action_property_changed(PropertyChangeEvent &e) {
  if (e.property_name == Action::ACCELERATOR) {
    set_accelerator(this->action->get_accelerator());
  } else {
    base::action_property_changed(e);
  }
}

}
