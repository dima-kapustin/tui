#pragma once

#include <tui++/event/Event.h>

namespace tui {

class Window;

class WindowEvent: public Event {
public:
  enum Type {
    WINDOW_OPENED = event_id_v<EventType::WINDOW, 0>,
    WINDOW_CLOSING = event_id_v<EventType::WINDOW, 1>,
    WINDOW_CLOSED = event_id_v<EventType::WINDOW, 2>,
    WINDOW_ACTIVATED = event_id_v<EventType::WINDOW, 3>,
    WINDOW_DEACTIVATED = event_id_v<EventType::WINDOW, 4>,
    WINDOW_GAINED_FOCUS = event_id_v<EventType::WINDOW, 5>,
    WINDOW_LOST_FOCUS = event_id_v<EventType::WINDOW, 6>,
    WINDOW_ICONIFIED = event_id_v<EventType::WINDOW, 7>,
    WINDOW_DEICONIFIED = event_id_v<EventType::WINDOW, 8>,
    /**
     * The window-state-changed event type.  This event is delivered
     * when a Window's state is changed by virtue of it being
     * iconified, maximized etc.
     */
    WINDOW_STATE_CHANGED = event_id_v<EventType::WINDOW, 9>
  };

public:
  /**
   * The other Window involved in this focus or activation change.
   * For a WINDOW_ACTIVATED or WINDOW_GAINED_FOCUS event, this is the Window
   * that lost activation or focus. For a WINDOW_DEACTIVATED or
   * WINDOW_LOST_FOCUS event, this is the Window that gained activation or
   * focus. For any other type of WindowEvent, or if the focus or activation
   * change occurs with a native application, or with no other Window,
   * null is returned.
   */
  std::shared_ptr<Window> opposite_window;

public:
  constexpr WindowEvent(const std::shared_ptr<Window> &source, Type type, const std::shared_ptr<Window> &opposite_window);
  constexpr WindowEvent(const std::shared_ptr<Window> &source, Type type);

  constexpr Type type() const {
    return Type(std::underlying_type_t<Type>(this->id));
  }

  constexpr std::shared_ptr<Window> get_window() const;
};

}
