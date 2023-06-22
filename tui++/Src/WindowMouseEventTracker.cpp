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

        this->target_last_entered = retarget_mouse_enter_exit(target_over, e, target_last_entered.lock(), this->isMouseInNativeContainer);
      }
    }, e);

  }
  return false;
}

std::shared_ptr<Component> WindowMouseEventTracker::retarget_mouse_enter_exit(const std::shared_ptr<Component> &target_over, Event &e, std::shared_ptr<Component> last_entered, bool in_window) {
  auto target_enter = in_window ? target_over : nullptr;

//  if (last_entered != target_enter) {
//    if (last_entered) {
//      retarget_mouse_event(last_entered, MouseOverEvent::MOUSE_EXITED, e);
//    }
//
//    if (id == MouseOverEvent::MOUSE_EXITED) {
//      // consume native exit event if we generate one
//      e.consume();
//    }
//
//    if (target_enter) {
//      retarget_mouse_event(target_enter, MouseOverEvent::MOUSE_ENTERED, e);
//    }
//
//    if (id == MouseOverEvent::MOUSE_ENTERED) {
//      // consume native enter event if we generate one
//      e.consume();
//    }
//  }
  return target_enter;
}

void WindowMouseEventTracker::start_listening_for_other_drags() {
  constexpr auto event_mask = EventType::MOUSE | EventType::MOUSE_MOVE | EventType::MOUSE_DRAG | EventType::MOUSE_OVER | EventType::MOUSE_WHEEL;
  this->window->get_screen()->add_event_listener(shared_from_this(), event_mask);
}

void WindowMouseEventTracker::stop_listening_for_other_drags() {
  this->window->get_screen()->remove_event_listener(shared_from_this());
}

void WindowMouseEventTracker::event_dispatched(Event &e) {

}

}
