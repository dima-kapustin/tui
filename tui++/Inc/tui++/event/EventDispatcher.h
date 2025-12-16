#pragma once

#include <tui++/event/EventListener.h>
#include <tui++/event/event_traits.h>

namespace tui {

class EventDispatcher {
public:
  static void dispatch_event(const std::shared_ptr<EventListener<ActionEvent>> &listener, ActionEvent &e) {
    listener->action_performed(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<KeyEvent>> &listener, KeyEvent &e) {
    switch (e.id) {
    case KeyEvent::KEY_PRESSED:
      listener->key_pressed(e);
      break;

    case KeyEvent::KEY_TYPED:
      listener->key_typed(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MousePressEvent>> &listener, MousePressEvent &e) {
    switch (e.id) {
    case MousePressEvent::MOUSE_PRESSED:
      listener->mouse_pressed(e);
      break;

    case MousePressEvent::MOUSE_RELEASED:
      listener->mouse_released(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseClickEvent>> &listener, MouseClickEvent &e) {
    listener->mouse_clicked(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseMoveEvent>> &listener, MouseMoveEvent &e) {
    listener->mouse_moved(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseDragEvent>> &listener, MouseDragEvent &e) {
    listener->mouse_dragged(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseWheelEvent>> &listener, MouseWheelEvent &e) {
    listener->mouse_wheel_moved(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<MouseOverEvent>> &listener, MouseOverEvent &e) {
    switch (e.id) {
    case MouseOverEvent::MOUSE_ENTERED:
      listener->mouse_entered(e);
      break;

    case MouseOverEvent::MOUSE_EXITED:
      listener->mouse_exited(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<ItemEvent>> &listener, ItemEvent &e) {
    listener->item_state_changed(e);
  }

  static void dispatch_event(const std::shared_ptr<EventListener<FocusEvent>> &listener, FocusEvent &e) {
    switch (e.id) {
    case FocusEvent::FOCUS_GAINED:
      listener->focus_gained(e);
      break;

    case FocusEvent::FOCUS_LOST:
      listener->focus_lost(e);
      break;
    }
  }

  static void dispatch_event(const std::shared_ptr<EventListener<WindowEvent>> &listener, WindowEvent &e) {
    switch (e.id) {
    case WindowEvent::WINDOW_OPENED:
      listener->window_opened(e);
      break;
    case WindowEvent::WINDOW_CLOSING:
      listener->window_closing(e);
      break;
    case WindowEvent::WINDOW_CLOSED:
      listener->window_closed(e);
      break;
    case WindowEvent::WINDOW_ACTIVATED:
      listener->window_activated(e);
      break;
    case WindowEvent::WINDOW_DEACTIVATED:
      listener->window_deactivated(e);
      break;
    case WindowEvent::WINDOW_GAINED_FOCUS:
      listener->window_gained_focus(e);
      break;
    case WindowEvent::WINDOW_LOST_FOCUS:
      listener->window_lost_focus(e);
      break;
    case WindowEvent::WINDOW_ICONIFIED:
      listener->window_iconified(e);
      break;
    case WindowEvent::WINDOW_DEICONIFIED:
      listener->window_deiconified(e);
      break;
    case WindowEvent::WINDOW_STATE_CHANGED:
      listener->window_state_changed(e);
      break;
    }
  }

  template<typename Listener, typename Event = event_type_from_listener_t<Listener>>
  requires (std::is_convertible_v<Listener*, EventListener<Event>*>)
  static void dispatch_event(const std::shared_ptr<Listener> &listener, Event &e) {
    if constexpr (std::is_same_v<ActionEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<KeyEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<MousePressEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<MouseClickEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<MouseMoveEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<MouseDragEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<MouseWheelEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<MouseOverEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<ItemEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<FocusEvent, Event>) {
      dispatch_event(listener, e);
    } else if constexpr (std::is_same_v<WindowEvent, Event>) {
      dispatch_event(listener, e);
    }
  }

};
}
