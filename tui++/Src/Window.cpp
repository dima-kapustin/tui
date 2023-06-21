#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/KeyboardFocusManager.h>

namespace tui {

void Window::add_notify() {
  base::add_notify();
  this->mouse_event_tracker = std::make_unique<WindowMouseEventTracker>(this);
}

void Window::dispatch_event(Event &e) {
  constexpr auto tracker_event_mask = EventType::MOUSE | EventType::MOUSE_MOVE | EventType::MOUSE_OVER;
  if (this->mouse_event_tracker and tracker_event_mask & e.get_type()) {
    this->mouse_event_tracker->process_event(e);
    return;
  }
  base::dispatch_event(e);
}

void Window::paint_components(Graphics &g) {
  int x = get_x(), y = get_y();
  g.translate(x, y);
  auto clip_rect = g.get_clip_rect();
  g.clip_rect(0, 0, get_width(), get_height());
  base::paint_components(g);
  g.set_clip_rect(clip_rect);
  g.translate(-x, -y);
}

std::shared_ptr<Component> Window::get_focus_owner() const {
  return is_focused() ? KeyboardFocusManager::get_focus_owner() : nullptr;
}

std::shared_ptr<Component> Window::get_most_recent_focus_owner() const {
  if (is_focused()) {
    return get_focus_owner();
  } else if (auto most_recent = KeyboardFocusManager::get_most_recent_focus_owner(std::dynamic_pointer_cast<const Window>(shared_from_this()))) {
    return most_recent;
  } else {
    return is_focusable_window() ? get_focus_traversal_policy()->get_initial_component(std::dynamic_pointer_cast<const Window>(shared_from_this())) : nullptr;
  }
}

bool Window::is_focusable_window() const {
  // If a Window/Frame/Dialog was made non-focusable, then it is always
  // non-focusable.
  if (not this->focusable_window_state) {
    return false;
  }

  // All other tests apply only to Windows.
  // TODO
//  if (this instanceof Frame or this instanceof Dialog) {
//    return true;
//  }

  // A Window must have at least one Component in its root focus
  // traversal cycle to be focusable.
  if (not get_focus_traversal_policy()->get_default_component(shared_from_this())) {
    return false;
  }

  // A Window's nearest owning Frame or Dialog must be showing on the
  // screen.
  for (auto owner = get_owner(); owner; owner = owner->get_owner()) {
    // TODO
//    if (owner instanceof Frame || owner instanceof Dialog) {
    return owner->is_showing();
//    }
  }

  return false;
}

void Window::set_focusable_window_state(bool state) {
  bool old_focusable_window_state = this->focusable_window_state;

  this->focusable_window_state = state;

  if (old_focusable_window_state and not this->focusable_window_state and is_focused()) {
    for (auto owner = get_owner(); owner; owner = owner->get_owner()) {
      auto to_focus = KeyboardFocusManager::get_most_recent_focus_owner(owner);
      if (to_focus and to_focus->request_focus(false, FocusEvent::Cause::ACTIVATION)) {
        return;
      }
    }

    KeyboardFocusManager::clear_focus_owner();
  }
}

void Window::to_front() {
  this->screen.to_front(std::dynamic_pointer_cast<Window>(shared_from_this()));
}

void Window::show() {
  validate_unconditionally();
  this->in_show = true;
  if (this->visible) {
    to_front();
  } else {
    this->before_first_show = false;
    // TODO
    // close_splash_screen();
    // Dialog::check_should_be_blocked(this);
    base::show();
    this->screen.show_window(std::dynamic_pointer_cast<Window>(shared_from_this()));
  }
  this->in_show = false;

  if (not this->opened) {
    post_event<WindowEvent>(WindowEvent::WINDOW_OPENED);
    this->opened = true;
  }
}

void Window::hide() {
  this->screen.hide_window(std::dynamic_pointer_cast<Window>(shared_from_this()));
  base::hide();
}

}
