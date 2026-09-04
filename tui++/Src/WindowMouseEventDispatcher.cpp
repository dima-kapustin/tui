#include <tui++/Event.h>
#include <tui++/Window.h>
#include <tui++/WindowMouseEventDispatcher.h>

#include <tui++/util/log.h>
#include <tui++/util/typeid.h>

namespace tui {

WindowMouseEventDispatcher::~WindowMouseEventDispatcher() {
  // The dispatcher only registered itself with the screen while a drag was
  // in progress; the screen holds a strong reference while it is registered,
  // so during teardown the last reference may be the one being released and
  // shared_from_this() would throw. Skip the removal when unregistered.
  if (not this->weak_from_this().expired()) {
    stop_listening_for_other_drags();
  }
}

void WindowMouseEventDispatcher::retarget_mouse_event(const std::shared_ptr<Component> &target, MouseEvent &e) {
  auto window = this->window.lock();
  if (not window) {
    // The owning window is gone (e.g. a closed popup); nothing to retarget to.
    return;
  }

  auto c = target;
  for (; c and c.get() != window.get(); c = c->get_parent()) {
    e.x -= c->get_x();
    e.y -= c->get_y();
  }

  if (c) {
    // Swing retargets the event to `target` in source-local coordinates: the
    // coordinates above were translated into `target`'s space, so update the
    // event's source to match. Listeners (including this dispatcher, when it
    // observes another window's events via the screen) can then convert to
    // screen space with convert_point_to_screen(e.x, e.y, source).
    e.source = target;
    if (target.get() == window.get()) {
      // avoid recursive calls
      window->dispatch_event_to_self(e);
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

void WindowMouseEventDispatcher::track_mouse_enter_exit(const std::shared_ptr<Component> &target_over, MouseEvent &e, bool inside_window) {
  if (inside_window != this->mouse_over_window) {
    this->mouse_over_window = inside_window;
    if (inside_window) {
      start_listening_for_other_drags();
    } else {
      stop_listening_for_other_drags();
    }
  }

  this->target_last_entered = retarget_mouse_enter_exit(target_over, e, target_last_entered.lock(), inside_window);
}

bool WindowMouseEventDispatcher::dispatch_event(Event &e) {
  if (this->event_mask & e.id) {
    dispatch_event(static_cast<MouseEvent&>(e));
  }
  return false;
}

bool WindowMouseEventDispatcher::dispatch_event(MouseEvent &e) {
  auto window = this->window.lock();
  if (not window) {
    // The owning window is gone; nothing left to hit-test or dispatch to.
    return false;
  }

  auto mouse_over = window->get_mouse_event_target(e.x, e.y, true);
  track_mouse_enter_exit(mouse_over, e, true);

  auto mouse_event_target = this->mouse_event_target.lock();
  // 4508327 : MOUSE_CLICKED should only go to the recipient of
  // the accompanying MOUSE_PRESSED, so don't reset mouse_event_target on a
  // MOUSE_CLICKED.
  if (not e.was_button_down_before() and e.id != MouseClickEvent::MOUSE_CLICKED) {
    mouse_event_target = mouse_over;
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
  } else {
    // No component under the pointer accepts mouse events: the window's
    // chrome/background, gaps between widgets, panels that only paint (like
    // the demo's hover panel), ... Still dispatch the event to the window
    // itself so screen listeners (e.g. the hover tracker) observe every
    // press/release/move/drag inside the window instead of the event
    // vanishing the moment the pointer leaves an interactive component.
    switch (e.id) {
    case MouseOverEvent::MOUSE_ENTERED:
    case MouseOverEvent::MOUSE_EXITED:
    case MouseClickEvent::MOUSE_CLICKED:
      // Enter/exit are synthesized for real components only; a click needs a
      // MOUSE_PRESSED recipient.
      break;
    case MousePressEvent::MOUSE_PRESSED:
    case MousePressEvent::MOUSE_RELEASED:
    case MouseMoveEvent::MOUSE_MOVED:
    case MouseDragEvent::MOUSE_DRAGGED:
      retarget_mouse_event(window, e);
      break;
    case MouseWheelEvent::MOUSE_WHEEL:
      break;
    }
    if (e.id != MouseWheelEvent::MOUSE_WHEEL) {
      e.consumed = true;
    }
  }

  return e.consumed;
}

void WindowMouseEventDispatcher::start_listening_for_other_drags() {
  constexpr auto event_mask = EventType::MOUSE_PRESS | EventType::MOUSE_MOVE | EventType::MOUSE_DRAG | EventType::MOUSE_OVER | EventType::MOUSE_WHEEL;
  screen.add_listener(event_mask, shared_from_this());
}

void WindowMouseEventDispatcher::stop_listening_for_other_drags() {
  screen.remove_listener(shared_from_this());
}

void WindowMouseEventDispatcher::event_dispatched(Event &e) {
  // This dispatcher is registered as a screen listener while the pointer is
  // over this window, to notice when it moves into another window (e.g. from
  // the menu bar into a popup). Synthesized enter/exit events are handled by
  // the window that generated them, so re-tracking them here would recurse;
  // ignore them.
  if (e.id == MouseOverEvent::MOUSE_ENTERED or e.id == MouseOverEvent::MOUSE_EXITED) {
    return;
  }

  // Only movement changes which component the pointer is over.
  if (e.id != MouseMoveEvent::MOUSE_MOVED and e.id != MouseDragEvent::MOUSE_DRAGGED) {
    return;
  }

  auto &mouse = static_cast<MouseEvent&>(e);

  auto window = this->window.lock();
  if (not window) {
    // The window this dispatcher serves is gone (it was hidden/destroyed
    // while the pointer was over it, e.g. a closed popup). The screen still
    // holds this dispatcher in its listener list, so drop it and stop
    // observing before touching the dangling window.
    stop_listening_for_other_drags();
    return;
  }

  // Swing keeps events in source-local coordinates: retarget_mouse_event has
  // already translated (x, y) into the hovered component's space and set the
  // event source to that component. Convert to this window's space here so
  // the enter/exit hit-test runs against this window's own components.
  auto source = std::dynamic_pointer_cast<Component>(e.source);
  if (not source) {
    return;
  }

  auto screen_point = convert_point_to_screen(mouse.x, mouse.y, source);

  // The pointer is over this window only if this window is topmost at the
  // point. A popup (heavyweight window) overlaps the frame beneath it, so a
  // bounds check alone would keep the frame tracking the pointer while it is
  // actually over the popup.
  auto inside_window = screen.get_window_at(screen_point).get() == window.get();
  auto local = convert_point_from_screen(screen_point, window->shared_from_this());

  auto saved_x = mouse.x;
  auto saved_y = mouse.y;
  mouse.x = local.x;
  mouse.y = local.y;

  auto target_over = inside_window ? window->get_mouse_event_target(local.x, local.y, true) : nullptr;
  track_mouse_enter_exit(target_over, mouse, inside_window);

  mouse.x = saved_x;
  mouse.y = saved_y;
}

}
