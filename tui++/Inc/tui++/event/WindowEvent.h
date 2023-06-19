#pragma once

#include <tui++/event/BasicEvent.h>

namespace tui {

class Window;

class WindowEvent: public BasicEvent {
public:
  enum Type {
    WINDOW_OPENED,
    WINDOW_CLOSING,
    WINDOW_CLOSED,
    WINDOW_ACTIVATED,
    WINDOW_DEACTIVATED,
    WINDOW_GAINED_FOCUS,
    WINDOW_LOST_FOCUS,
    WINDOW_ICONIFIED,
    WINDOW_DEICONIFIED,
    /**
     * The window-state-changed event type.  This event is delivered
     * when a Window's state is changed by virtue of it being
     * iconified, maximized etc.
     */
    WINDOW_STATE_CHANGED
  };

public:
  const Type type;
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

  constexpr std::shared_ptr<Window> get_window() const;
};

}
