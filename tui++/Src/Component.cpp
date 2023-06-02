#include <tui++/Border.h>
#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/Component.h>

#include <tui++/Screen.h>

namespace tui {

std::shared_ptr<Window> Component::get_window_ancestor() const {
  if (auto window = std::dynamic_pointer_cast<Window>(const_cast<Component*>(this)->shared_from_this())) {
    return window;
  }

  for (auto ancestor = get_parent(); ancestor; ancestor = ancestor->get_parent()) {
    if (auto window = std::dynamic_pointer_cast<Window>(ancestor)) {
      return window;
    }
  }
  return {};
}

void Component::paint_border(Graphics &g) {
  if (this->border) {
    this->border->paint_border(*this, g, 0, 0, get_width(), get_height());
  }
}

void Component::paint_children(Graphics &g) {
  for (auto &&c : this->components) {
    if (c->is_visible()) {
      int x = c->get_x(), y = c->get_y();
      g.translate(x, y);
      auto clip_rect = g.get_clip_rect();
      g.clip_rect(0, 0, c->get_width(), c->get_height());
      c->paint(g);
      g.set_clip_rect(clip_rect);
      g.translate(-x, -y);
    }
  }
}

void Component::request_focus() {
  if (auto focus_component = get_focus_component(); not focus_component or not has_children()) {
    auto temporary = false;

    // generate the FOCUS_GAINED only if the component does not
    // already have the focus

    auto ancestor = get_window_ancestor();
    assert(ancestor != nullptr);        // "Cannot request_focus before the component is added to a window");

    auto ancestor_current_focus = ancestor->get_focus_component();
    auto top_window = Screen::get_top_window();
    auto top_window_current_focus = top_window->get_focus_component();

    if (ancestor_current_focus != shared_from_this() or (top_window_current_focus == shared_from_this() and not this->was_focus_owner)) {
      this->was_focus_owner = true;

      if (auto lastEvt = Screen::get_last_focus_event()) {
        ancestor_current_focus = lastEvt->focus.source;
        if (ancestor_current_focus and ancestor_current_focus->get_window_ancestor() and ancestor_current_focus->get_window_ancestor()->is_enabled() and ancestor_current_focus->get_window_ancestor() != ancestor) {
          temporary = true;
        } else {
          temporary = false;
        }

        Screen::post(std::make_shared<Event>(ancestor_current_focus, FocusEvent::FOCUS_LOST, temporary, shared_from_this()));
      } else {
        ancestor_current_focus = nullptr;
      }

      Screen::post(std::make_shared<Event>(shared_from_this(), FocusEvent::FOCUS_GAINED, temporary, ancestor_current_focus));

      if (auto parent = get_parent()) {
        parent->set_focus(shared_from_this());
      }

      repaint();
    }
  } else {
    focus_component->request_focus();
  }
}

/**
 * Convert a point from a screen coordinates to a component's coordinate system
 */
Point convert_point_from_screen(int x, int y, std::shared_ptr<Component> to) {
  do {
    x -= to->get_x();
    y -= to->get_y();
    to = to->get_parent();
  } while (to);
  return {x, y};
}

/**
 * Convert a point from a component's coordinate system to screen coordinates.
 */
Point convert_point_to_screen(int x, int y, std::shared_ptr<Component> from) {
  while (from) {
    x += from->get_x();
    y += from->get_y();
    from = from->get_parent();
  }
  return {x, y};
}
}
