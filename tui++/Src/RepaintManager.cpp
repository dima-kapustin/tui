#include <tui++/RepaintManager.h>

#include <tui++/Frame.h>
#include <tui++/Screen.h>
#include <tui++/Window.h>
#include <tui++/Component.h>

#include <tui++/event/InvocationEvent.h>

namespace tui {

RepaintManager::RepaintManager() {

}

void RepaintManager::add_dirty_region(std::shared_ptr<Component> const &c, Rectangle const &bounds) {
  if (bounds.empty() or not c or c->get_width() <= 0 or c->get_height() <= 0) {
    return;
  } else if (extend_dirty_region(c, bounds)) {
    return;
  }

  auto root = std::shared_ptr<Window> { };
  for (auto p = c; p; p = p->get_parent()) {
    if (not p->is_visible() or not p->is_displayable()) {
      return;
    } else if (auto window = std::dynamic_pointer_cast<Window>(p)) {
      if (auto frame = std::dynamic_pointer_cast<Frame>(window)) {
//        if (p->is_iconified()) {
//          return;
//        }
      }
      root = window;
      break;
    }
  }

  if (not root) {
    return;
  }

  std::unique_lock lock { this->mutex };
  if (auto &dirty_bounds = this->dirty_regions[c]; not dirty_bounds.empty()) {
    dirty_bounds |= bounds;
    return;
  } else {
    dirty_bounds = bounds;
    screen.post([self = shared_from_this()] {
      self->repaint_dirty_regions();
    });
  }
}

bool RepaintManager::extend_dirty_region(std::shared_ptr<Component> const &c, Rectangle const &bounds) {
  if (auto pos = this->dirty_regions.find(c); pos != this->dirty_regions.end()) {
    pos->second |= bounds;
    return true;
  }
  return false;
}

void RepaintManager::repaint_dirty_regions() {

}

}
