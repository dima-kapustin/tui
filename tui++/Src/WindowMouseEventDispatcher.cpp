#include <tui++/Event.h>
#include <tui++/Window.h>
#include <tui++/WindowMouseEventDispatcher.h>

#include <tui++/util/log.h>
#include <tui++/util/typeid.h>

namespace tui {

WindowMouseEventDispatcher::~WindowMouseEventDispatcher() {
  stop_listening_for_other_drags();
}

void WindowMouseEventDispatcher::retarget_mouse_event(const std::shared_ptr<Component> &target, MouseEvent &e) {
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

std::shared_ptr<Component> WindowMouseEventDispatcher::retarget_mouse_enter_exit(const std::shared_ptr<Component> &target_over, MouseEvent &e, const std::shared_ptr<Component> &last_entered, bool mouse_over_window) {
  auto target_enter = mouse_over_window ? target_over : nullptr;

  if (last_entered != target_enter) {
    if (last_entered) {
      auto mouse_exited = make_event<MouseOverEvent>(last_entered, MouseOverEvent::MOUSE_EXITED, e.modifiers, e.x, e.y, e.when);
      retarget_mouse_event(last_entered, mouse_exited);
    }

    if (e.id == MouseOverEvent::MOUSE_EXITED) {
      // consume native exit event if we generate one
      e.consumed = true;
    }

    if (target_enter) {
      auto mouse_entered = make_event<MouseOverEvent>(target_enter, MouseOverEvent::MOUSE_ENTERED, e.modifiers, e.x, e.y, e.when);
      retarget_mouse_event(target_enter, mouse_entered);
    }

    if (e.id == MouseOverEvent::MOUSE_ENTERED) {
      // consume native enter event if we generate one
      e.consumed = true;
    }
  }
  return target_enter;
}

void WindowMouseEventDispatcher::track_mouse_enter_exit(const std::shared_ptr<Component> &target_over, MouseEvent &e) {
  if (e.id != MouseOverEvent::MOUSE_EXITED and //
      e.id != MouseDragEvent::MOUSE_DRAGGED and //
      not this->mouse_over_window) {
    // any event but an exit or drag means we're in the window
    this->mouse_over_window = true;
    start_listening_for_other_drags();
  } else if (e.id == MouseOverEvent::MOUSE_EXITED) {
    this->mouse_over_window = false;
    stop_listening_for_other_drags();
  }

  this->target_last_entered = retarget_mouse_enter_exit(target_over, e, target_last_entered.lock(), this->mouse_over_window);
}

bool WindowMouseEventDispatcher::dispatch_event(Event &e) {
  if (this->event_mask & e.id) {
    dispatch_event(static_cast<MouseEvent&>(e));
  }
  return false;
}

bool WindowMouseEventDispatcher::dispatch_event(MouseEvent &e) {
  auto mouse_over = this->window->get_mouse_event_target(e.x, e.y, true);
  track_mouse_enter_exit(mouse_over, e);

  auto mouse_event_target = this->mouse_event_target.lock();
  // 4508327 : MOUSE_CLICKED should only go to the recipient of
  // the accompanying MOUSE_PRESSED, so don't reset mouse_event_target on a
  // MOUSE_CLICKED.
  if (not e.was_button_down_before() and e.id != MouseClickEvent::MOUSE_CLICKED) {
    mouse_event_target = mouse_over; // (mouse_over.get() != this->window) ? mouse_over : nullptr;
    this->mouse_event_target = mouse_event_target;
  }

  if (mouse_event_target) {
    switch (e.id) {
    case MouseOverEvent::MOUSE_ENTERED:
    case MouseOverEvent::MOUSE_EXITED:
      break;
    case MousePressEvent::MOUSE_PRESSED:
      retarget_mouse_event(mouse_event_target, e);
      break;
    case MousePressEvent::MOUSE_RELEASED:
      retarget_mouse_event(mouse_event_target, e);
      break;
    case MouseClickEvent::MOUSE_CLICKED:
      // 4508327: MOUSE_CLICKED should never be dispatched to a Component
      // other than that which received the MOUSE_PRESSED event.  If the
      // mouse is now over a different Component, don't dispatch the event.
      // The previous fix for a similar problem was associated with bug
      // 4155217.
      if (mouse_over == mouse_event_target) {
        retarget_mouse_event(mouse_over, e);
      }
      break;
    case MouseMoveEvent::MOUSE_MOVED:
      retarget_mouse_event(mouse_event_target, e);
      break;
    case MouseDragEvent::MOUSE_DRAGGED:
      if (e.was_button_down_before()) {
        retarget_mouse_event(mouse_event_target, e);
      }
      break;
    case MouseWheelEvent::MOUSE_WHEEL:
      // This may send it somewhere that doesn't have MouseWheelEvents
      // enabled.  In this case, Component::dispatch_event() will
      // retarget the event to a parent that DOES have the events enabled.
      log_event_ln("retargeting mouse wheel to " << mouse_over->get_name() << ", " << typeid(*mouse_over.get()));
      retarget_mouse_event(mouse_over, e);
      break;
    }
    //Consuming of wheel events is implemented in "retargetMousePressEvent".
    if (e.id != MouseWheelEvent::MOUSE_WHEEL) {
      e.consumed = true;
    }
  }

  return e.consumed;
}

void WindowMouseEventDispatcher::start_listening_for_other_drags() {
  constexpr auto event_mask = EventType::MOUSE_PRESS | EventType::MOUSE_MOVE | EventType::MOUSE_DRAG | EventType::MOUSE_OVER | EventType::MOUSE_WHEEL;
  this->window->get_screen()->add_event_listener(shared_from_this(), event_mask);
}

void WindowMouseEventDispatcher::stop_listening_for_other_drags() {
  this->window->get_screen()->remove_event_listener(shared_from_this());
}

void WindowMouseEventDispatcher::event_dispatched(Event &e) {

}

}
