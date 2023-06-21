#pragma once

#include <tui++/event/EventListener.h>
#include <tui++/event/event_traits.h>

namespace tui {

class EventDispatcher {
public:
  static void dispatch_event(const std::shared_ptr<EventListener<ActionEvent>> &event_listener, ActionEvent &e) {
    event_listener->action_performed(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<KeyEvent>> &event_listener, KeyEvent &e) {
    switch (e.type) {
    case KeyEvent::KEY_PRESSED:
      event_listener->key_pressed(e);
      break;

    case KeyEvent::KEY_TYPED:
      event_listener->key_typed(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseEvent>> &event_listener, MouseEvent &e) {
    switch (e.type) {
    case MouseEvent::MOUSE_PRESSED:
      event_listener->mouse_pressed(e);
      break;

    case MouseEvent::MOUSE_RELEASED:
      event_listener->mouse_released(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseClickEvent>> &event_listener, MouseClickEvent &e) {
    event_listener->mouse_clicked(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseMoveEvent>> &event_listener, MouseMoveEvent &e) {
    event_listener->mouse_moved(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseDragEvent>> &event_listener, MouseDragEvent &e) {
    event_listener->mouse_dragged(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseWheelEvent>> &event_listener, MouseWheelEvent &e) {
    event_listener->mouse_wheel_moved(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseOverEvent>> &event_listener, MouseOverEvent &e) {
    switch (e.type) {
    case MouseOverEvent::MOUSE_ENTERED:
      event_listener->mouse_entered(e);
      break;

    case MouseOverEvent::MOUSE_EXITED:
      event_listener->mouse_exited(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<ItemEvent>> &event_listener, ItemEvent &e) {
    event_listener->item_state_changed(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<FocusEvent>> &event_listener, FocusEvent &e) {
    switch (e.type) {
    case FocusEvent::FOCUS_GAINED:
      event_listener->focus_gained(e);
      break;

    case FocusEvent::FOCUS_LOST:
      event_listener->focus_lost(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<WindowEvent>> &event_listener, WindowEvent &e) {
    switch (e.type) {
    case WindowEvent::WINDOW_OPENED:
      event_listener->window_opened(e);
      break;
    case WindowEvent::WINDOW_CLOSING:
      event_listener->window_closing(e);
      break;
    case WindowEvent::WINDOW_CLOSED:
      event_listener->window_closed(e);
      break;
    case WindowEvent::WINDOW_ACTIVATED:
      event_listener->window_activated(e);
      break;
    case WindowEvent::WINDOW_DEACTIVATED:
      event_listener->window_deactivated(e);
      break;
    case WindowEvent::WINDOW_GAINED_FOCUS:
      event_listener->window_gained_focus(e);
      break;
    case WindowEvent::WINDOW_LOST_FOCUS:
      event_listener->window_lost_focus(e);
      break;
    case WindowEvent::WINDOW_ICONIFIED:
      event_listener->window_iconified(e);
      break;
    case WindowEvent::WINDOW_DEICONIFIED:
      event_listener->window_deiconified(e);
      break;
    case WindowEvent::WINDOW_STATE_CHANGED:
      event_listener->window_state_changed(e);
      break;
    }
  }

  template<typename Listener, typename Event = event_type_from_listener_t<Listener>>
  requires (std::is_convertible_v<Listener*, EventListener<Event>*>)
  static void dispatch_event(const std::shared_ptr<Listener> &event_listener, Event &e) {
    if constexpr (std::is_same_v<ActionEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<KeyEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<MouseEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<MouseClickEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<MouseMoveEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<MouseDragEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<MouseWheelEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<MouseOverEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<ItemEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<FocusEvent, Event>) {
      dispatch_event(event_listener, e);
    } else if constexpr (std::is_same_v<WindowEvent, Event>) {
      dispatch_event(event_listener, e);
    }
  }

};
}
