#include <tui++/Menu.h>

#include <tui++/lookandfeel/MenuUI.h>

namespace tui {

void Menu::init() {
  this->popup_menu = make_component<PopupMenu>();
  this->popup_menu->set_invoker(shared_from_this());
}

std::shared_ptr<laf::MenuUI> Menu::get_ui() const {
  return std::static_pointer_cast<laf::MenuUI>(this->ui.value());
}

void Menu::update_ui() {
  base::update_ui();
  if (this->popup_menu) {
    this->popup_menu->update_ui();
  }
}

std::shared_ptr<laf::ComponentUI> Menu::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

void Menu::init_focusability() {
  // do nothing to keep menu focusable
}

bool Menu::is_popup_menu_visible() const {
  return this->popup_menu->is_visible();
}

void Menu::set_popup_menu_visible(bool value) {
  auto isVisible = is_popup_menu_visible();
  if (isVisible != value and (is_enabled() or not value)) {
    if (value and is_showing()) {
      // Set location of popupMenu (pulldown or pullright)
      auto p = this->custom_menu_location.has_value() ? this->custom_menu_location.value() : get_popup_menu_origin();
      this->popup_menu->show(shared_from_this(), p.x, p.y);
    } else {
      this->popup_menu->set_visible(false);
    }
  }
}

Point Menu::get_popup_menu_origin() const {
  return {};
}

void Menu::set_menu_location(Point const &p) {
  this->custom_menu_location = p;
  this->popup_menu->set_location(p);
}

void Menu::state_changed(ChangeEvent &e) {
  base::state_changed(e);
  if (auto is_selected = get_model()->is_selected(); is_selected != this->is_selected) {
    fire_event<MenuEvent>(std::static_pointer_cast<Menu>(shared_from_this()), is_selected ? MenuEvent::SELECTED : MenuEvent::DESELECTED);
    this->is_selected = is_selected;
  }
}

}
