#include <tui++/WindowMouseEventTracker.h>
#include <tui++/Window.h>

namespace tui {

bool WindowMouseEventTracker::process_event(Event &e) {
  if (this->window->is_event_enabled(e.get_type())) {
    std::visit([&e, this](auto &&arg) {
      using Arg = std::decay_t<decltype(arg)>();
      if constexpr (std::is_convertible_v<Arg*, MouseEventBase*>) {
        auto mouse_event = static_cast<MouseEventBase&>(arg);
        auto target_over = this->window->get_mouse_event_target(mouse_event.x, mouse_event.y, true);

        constexpr auto is_mouse_exited = std::is_same_v<Arg, MouseOverEvent> and arg.type == MouseOverEvent::MOUSE_EXITED;

        if (not is_mouse_exited and //
            not std::is_same_v<Arg, MouseDragEvent> and //
            not this->isMouseInNativeContainer) {
          // any event but an exit or drag means we're in the native container
          this->isMouseInNativeContainer = true;
          start_listening_for_other_drags();
        } else if (is_mouse_exited) {
          this->isMouseInNativeContainer = false;
          stop_listening_for_other_drags();
        }

//        Component tle = retargetMouseEnterExit(targetOver, e, targetLastEntered.get(), isMouseInNativeContainer);
//        targetLastEntered = new WeakReference<>(tle);
      }
    }, e);

  }
  return false;
}

void WindowMouseEventTracker::start_listening_for_other_drags() {
  this->window->get_screen()->add_event_listener(shared_from_this(), EventType::MOUSE | EventType::MOUSE_MOVE | EventType::MOUSE_DRAG | EventType::MOUSE_OVER);
}

void WindowMouseEventTracker::stop_listening_for_other_drags() {
  this->window->get_screen()->remove_event_listener(shared_from_this());
}

void WindowMouseEventTracker::event_dispatched(Event &e) {

}

}
