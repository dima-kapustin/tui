#include <tui++/PopupWindow.h>

namespace tui {

void PopupWindow::reanchor() {
  if (not is_showing()) {
    return;
  }

  if (auto component = this->anchor.lock()) {
    // The component the popup hangs off may itself sit in a popup (a submenu
    // hangs off a row of its parent menu), and it is the parent's own new size
    // that lays that row out, so the parent is placed first.
    if (auto anchor_window = component->get_containing_window()) {
      if (auto parent = std::dynamic_pointer_cast<PopupWindow>(anchor_window); parent and parent.get() != this) {
        parent->reanchor();
      }

      // The anchor may have been laid out again by the screen-wide change this
      // re-places after; a valid tree is what its location is read from.
      anchor_window->validate();
    }

    if (component->is_showing()) {
      auto origin = component->get_location_on_screen();
      set_location(origin.x + this->anchor_offset.x, origin.y + this->anchor_offset.y);
    }
  }

  // A popup is sized to its contents (PopupWindow::show packs it), and the
  // items may have changed size even with the anchor standing still.
  pack();
}

}
