#pragma once

#include <tui++/event/InputEvent.h>

namespace tui {

class MouseEvent: public InputEvent {
public:
  enum Type {
    MOUSE_PRESSED,
    MOUSE_RELEASED,
    MOUSE_CLICKED,
    MOUSE_WHEEL,
    MOUSE_MOVED,
    MOUSE_DRAGGED
  };

  enum Button {
    LEFT_BUTTON = 0,
    MIDDLE_BUTTON = 1,
    RIGHT_BUTTON = 2,
    NO_BUTTON = 3
  };

public:
  const Type type;
  Button button = NO_BUTTON;
  int x, y;

  union {
    unsigned click_count;
    // The number of "clicks" the mouse wheel was rotated.
    // Negative values denote mouse wheel rotation up/away from the user, and positive values denote mouse wheel was rotation down/towards the user.
    int weel_rotation;
  };
  bool is_popup_trigger;

public:
  constexpr MouseEvent(const std::shared_ptr<Component> &source, Type type, Button button, Modifiers modifiers, int x, int y, int click_count_or_wheel_rotation, bool is_popup_trigger, const EventClock::time_point &when = EventClock::now()) :
      InputEvent(source, modifiers, when), type(type), button(button), x(x), y(y), click_count(click_count_or_wheel_rotation), is_popup_trigger(is_popup_trigger) {
  }
};

}
