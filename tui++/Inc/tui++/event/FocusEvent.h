#pragma once

#include <tui++/event/Event.h>

namespace tui {

class FocusEvent: public Event {
public:
  enum Type : unsigned {
    FOCUS_LOST = event_id_v<EventType::FOCUS, 0>,
    FOCUS_GAINED = event_id_v<EventType::FOCUS, 1>
  };

  enum class Cause {
    /**
     * The default value.
     */
    UNKNOWN,
    /**
     * An activating mouse event.
     */
    MOUSE_EVENT,
    /**
     * A focus traversal action with unspecified direction.
     */
    TRAVERSAL,
    /**
     * An up-cycle focus traversal action.
     */
    TRAVERSAL_UP,
    /**
     * A down-cycle focus traversal action.
     */
    TRAVERSAL_DOWN,
    /**
     * A forward focus traversal action.
     */
    TRAVERSAL_FORWARD,
    /**
     * A backward focus traversal action.
     */
    TRAVERSAL_BACKWARD,
    /**
     * Restoring focus after a focus request has been rejected.
     */
    ROLLBACK,
    /**
     * A system action causing an unexpected focus change.
     */
    UNEXPECTED,
    /**
     * An activation of a toplevel window.
     */
    ACTIVATION,
    /**
     * Clearing global focus owner.
     */
    CLEAR_GLOBAL_FOCUS_OWNER
  };

public:
  const Cause cause;
  bool temporary;
  std::shared_ptr<Component> opposite;

public:
  FocusEvent(const std::shared_ptr<Component> &source, Type type, bool temporary, const std::shared_ptr<Component> &opposite) :
      FocusEvent(source, type, Cause::UNKNOWN, temporary, opposite) {
  }

  FocusEvent(const std::shared_ptr<Component> &source, Type type, Cause cause, bool temporary, const std::shared_ptr<Component> &opposite) :
      Event(source, type), cause(cause), temporary(temporary), opposite(opposite) {
  }
};

}
