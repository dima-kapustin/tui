#include <tui++/Menu.h>
#include <tui++/Screen.h>
#include <tui++/Window.h>

#include <tui++/lookandfeel/MenuUI.h>

namespace tui {

void Menu::init() {
  Component::init();
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

std::shared_ptr<MenuItem> Menu::add(std::string const &label) {
  return this->popup_menu->add(label);
}

std::shared_ptr<MenuItem> Menu::add(std::shared_ptr<Action> const &action) {
  return this->popup_menu->add(action);
}

void Menu::add_separator() {
  this->popup_menu->add_separator();
}

void Menu::add_impl(const std::shared_ptr<Component> &c, const Constraints &constraints, int z_order) {
  this->popup_menu->add(c, constraints, z_order);
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
  // Figure out the sizes needed to caclulate the menu position
  auto size = get_size();
  auto popup_menu_size = this->popup_menu->get_size();
  // For the first time the menu is popped up,
  // the size has not yet been initiated
  if (popup_menu_size.width == 0) {
    popup_menu_size = this->popup_menu->get_preferred_size();
  }
  auto position = get_location_on_screen();
  auto screen_bounds = Rectangle { { }, screen.get_size() };
  auto x = 0, y = 9;
  if (std::dynamic_pointer_cast<PopupMenu>(get_parent()) != nullptr) {
    // We are a submenu (pull-right)
    int x_offset = laf::LookAndFeel::get<int>("Menu.submenuPopupOffsetX");
    int y_offset = laf::LookAndFeel::get<int>("Menu.submenuPopupOffsetY");

    if (this->orientation.is_left_to_right()) {
      x = size.width + x_offset; // Prefer placement to the right
      if ((position.x + x + popup_menu_size.width) >= (screen_bounds.width + screen_bounds.x) and //
          (screen_bounds.width - size.width) < 2 * (position.x - screen_bounds.x)) {
        x = 0 - x_offset - popup_menu_size.width;
      }
    } else {
      x = 0 - x_offset - popup_menu_size.width; // Prefer placement to the left
      if ((position.x + x < screen_bounds.x) and //
          (screen_bounds.width - size.width) > 2 * (position.x - screen_bounds.x)) {
        x = size.width + x_offset;
      }
    }

    y = y_offset; // Prefer dropping down
    if ((position.y + y + popup_menu_size.height) >= (screen_bounds.height + screen_bounds.y) and //
        (screen_bounds.height - size.height) < 2 * (position.y - screen_bounds.y)) {
      y = size.height - y_offset - popup_menu_size.height;
    }
  } else {
    // We are a toplevel menu (pull-down)
    int x_offset = laf::LookAndFeel::get<int>("Menu.menuPopupOffsetX");
    int y_offset = laf::LookAndFeel::get<int>("Menu.menuPopupOffsetY");

    if (this->orientation.is_left_to_right()) {
      x = x_offset; // Extend to the right
      if ((position.x + x + popup_menu_size.width) >= (screen_bounds.width + screen_bounds.x) and //
          (screen_bounds.width - size.width) < 2 * (position.x - screen_bounds.x)) {
        x = size.width - x_offset - popup_menu_size.width;
      }
    } else {
      x = size.width - x_offset - popup_menu_size.width; // Extend to the left
      if ((position.x + x) < screen_bounds.x and //
          (screen_bounds.width - size.width) > 2 * (position.x - screen_bounds.x)) {
        x = x_offset;
      }
    }

    y = size.height + y_offset; // Prefer dropping down
    if ((position.y + y + popup_menu_size.height) >= (screen_bounds.height + screen_bounds.y) and //
        (screen_bounds.height - size.height) < 2 * (position.y - screen_bounds.y)) {
      y = 0 - y_offset - popup_menu_size.height;   // Otherwise drop 'up'
    }
  }
  return {x, y};
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
