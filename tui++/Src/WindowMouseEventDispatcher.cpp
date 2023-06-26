#include <tui++/Event.h>
#include <tui++/Window.h>
#include <tui++/WindowMouseEventDispatcher.h>

namespace tui {

void WindowMouseEventDispatcher::retarget_mouse_event(const std::shared_ptr<Component> &target, MouseEventBase &e) {
  auto component = target;
  for (; component and component.get() != this->window; component = component->get_parent()) {
    e.x -= component->get_x();
    e.y -= component->get_y();
  }

  if (component) {
    if (target.get() == this->window) {
      // avoid recursive calls
      this->window->dispatch_event_to_self(e);
    } else {
      // TODO
//        if (nativeContainer.modalComp != null) {
//            if (((Container)nativeContainer.modalComp).isAncestorOf(target)) {
//                target.dispatchEvent(retargeted);
//            } else {
//                e.consume();
//            }
//        } else {
      target->dispatch_event(e);
//        }
    }
  }
}

std::shared_ptr<Component> WindowMouseEventDispatcher::retarget_mouse_enter_exit(const std::shared_ptr<Component> &target_over, MouseEventBase &e, const std::shared_ptr<Component> &last_entered, bool mouse_over_window) {
  auto target_enter = mouse_over_window ? target_over : nullptr;

  if (last_entered != target_enter) {
    if (last_entered) {
      auto mouse_exited = make_event<MouseOverEvent>(last_entered, MouseOverEvent::MOUSE_EXITED, e.button, e.modifiers, e.x, e.y, e.when);
      retarget_mouse_event(last_entered, mouse_exited);
    }

    if (e.id == MouseOverEvent::MOUSE_EXITED) {
      // consume native exit event if we generate one
      e.consumed = true;
    }

    if (target_enter) {
      auto mouse_entered = make_event<MouseOverEvent>(target_enter, MouseOverEvent::MOUSE_ENTERED, e.button, e.modifiers, e.x, e.y, e.when);
      retarget_mouse_event(target_enter, mouse_entered);
    }

    if (e.id == MouseOverEvent::MOUSE_ENTERED) {
      // consume native enter event if we generate one
      e.consumed = true;
    }
  }
  return target_enter;
}

bool WindowMouseEventDispatcher::dispatch_event(MouseEventBase &e) {
  if (this->event_mask & e.id) {
    auto target_over = this->window->get_mouse_event_target(e.x, e.y, true);

    if (e.id != MouseOverEvent::MOUSE_EXITED and //
        e.id != MouseDragEvent::MOUSE_DRAGGED and //
        not this->mouse_over_window) {
      // any event but an exit or drag means we're in the native container
      this->mouse_over_window = true;
      start_listening_for_other_drags();
    } else if (e.id == MouseOverEvent::MOUSE_EXITED) {
      this->mouse_over_window = false;
      stop_listening_for_other_drags();
    }

    this->target_last_entered = retarget_mouse_enter_exit(target_over, e, target_last_entered.lock(), this->mouse_over_window);
  }
  return false;
}

void WindowMouseEventDispatcher::start_listening_for_other_drags() {
  constexpr auto event_mask = EventType::MOUSE | EventType::MOUSE_MOVE | EventType::MOUSE_DRAG | EventType::MOUSE_OVER | EventType::MOUSE_WHEEL;
  this->window->get_screen()->add_event_listener(shared_from_this(), event_mask);
}

void WindowMouseEventDispatcher::stop_listening_for_other_drags() {
  this->window->get_screen()->remove_event_listener(shared_from_this());
}

void WindowMouseEventDispatcher::event_dispatched(Event &e) {

}

}
