#include <tui++/Popup.h>
#include <tui++/PopupWindow.h>
#include <tui++/BorderLayout.h>
#include <tui++/ComboBox.h>
#include <tui++/PopupMenu.h>
#include <tui++/Shadow.h>

#include <tui++/lookandfeel/LookAndFeel.h>

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

namespace {

// The shadow a popup casts is the one themed for what opened it: the dropdown
// a combo box opens shades like the combo box, the popup a menu opens like a
// popup menu (the component the window shows), and any other popup like a
// popup window. The popup window itself cannot tell these apart -- it only
// knows it is a popup -- so the choice is made where both the invoker and the
// contents are known.
std::string_view get_shadow_key(std::shared_ptr<Component> const &owner, std::shared_ptr<Component> const &contents) {
  if (is_a<ComboBox>(owner)) {
    return "ComboBox.Shadow";
  } else if (is_a<PopupMenu>(contents)) {
    return "PopupMenu.Shadow";
  }
  return "PopupWindow.Shadow";
}

}

Popup::Popup(std::shared_ptr<Component> const &owner, std::shared_ptr<Component> const &contents, int owner_x, int owner_y) :
    window(make_component<PopupWindow>(get_parent_window(owner))) {
  assert(this->window);

  this->window->set_shadow(laf::LookAndFeel::get<std::optional<Shadow>>(get_shadow_key(owner, contents)));
  this->window->set_bounds(owner_x, owner_y, 1, 1);
  this->window->add(contents, BorderLayout::CENTER);

  // What the popup hangs off and where it sits relative to it: a screen-wide
  // relayout -- a translation resizes the items, the shadow switch moves the
  // components around them -- places the popup again from these (see
  // PopupWindow::reanchor). A popup opened without a component (at a point
  // only) stays where it was put.
  if (owner and owner->is_showing()) {
    auto origin = owner->get_location_on_screen();
    std::static_pointer_cast<PopupWindow>(this->window)->set_anchor(owner, Point { owner_x - origin.x, owner_y - origin.y });
  }

  this->window->invalidate();
  this->window->validate();
  if (this->window->is_visible()) {
    this->window->pack();
  }
}

void Popup::show() {
  this->window->set_visible(true);
}

void Popup::hide() {
  this->window->set_visible(false);
}

}
