#include <tui++/Popup.h>
#include <tui++/PopupWindow.h>
#include <tui++/BorderLayout.h>

#include <cassert>

namespace tui {

std::shared_ptr<Window> get_parent_window(std::shared_ptr<Component> const &owner) {
  if (auto window = std::dynamic_pointer_cast<Window>(owner)) {
    return window;
  }

  for (auto parent = owner->get_parent(); parent; parent = parent->get_parent()) {
    if (auto window = std::dynamic_pointer_cast<Window>(parent)) {
      return window;
    }
  }
  return {};
}

Popup::Popup(std::shared_ptr<Component> const &owner, std::shared_ptr<Component> const &contents, int owner_x, int owner_y) :
    window(make_component<PopupWindow>(get_parent_window(owner))) {
  assert(this->window);

  this->window->set_bounds(owner_x, owner_y, 1, 1);
  this->window->add(contents, BorderLayout::CENTER);
  this->window->invalidate();
  this->window->validate();
  if (this->window->is_visible()) {
    this->window->pack();
  }
}

void Popup::show() {

}

void Popup::hide() {

}

}
