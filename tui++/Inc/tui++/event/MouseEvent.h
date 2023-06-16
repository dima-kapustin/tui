#pragma once

#include <tui++/event/InputEvent.h>

namespace tui {

class MouseEventBase: public InputEvent {
public:
  enum Button {
    LEFT_BUTTON = 0,
    MIDDLE_BUTTON = 1,
    RIGHT_BUTTON = 2,
    NO_BUTTON = 3
  };

public:
  Button button = NO_BUTTON;
  int x, y;

protected:
  constexpr MouseEventBase(const std::shared_ptr<Component> &source, Button button, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      InputEvent(source, modifiers, when), button(button), x(x), y(y) {
  }
};

class MouseEvent: public MouseEventBase {
public:
  enum Type {
    MOUSE_PRESSED,
    MOUSE_RELEASED
  };

public:
  const Type type;
  bool is_popup_trigger;

public:
  constexpr MouseEvent(const std::shared_ptr<Component> &source, Type type, Button button, Modifiers modifiers, int x, int y, bool is_popup_trigger, const EventClock::time_point &when = EventClock::now()) :
      MouseEventBase(source, button, modifiers, x, y, when), type(type), is_popup_trigger(is_popup_trigger) {
  }
};

class MouseHoverEvent: public MouseEventBase {
public:
  enum Type {
    MOUSE_ENTERED,
    MOUSE_EXITED,
  };

public:
  const Type type;
public:
  constexpr MouseHoverEvent(const std::shared_ptr<Component> &source, Type type, Button button, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEventBase(source, button, modifiers, x, y, when), type(type) {
  }
};

class MouseClickEvent: public MouseEventBase {
public:
  unsigned click_count;
  bool is_popup_trigger;

public:
  constexpr MouseClickEvent(const std::shared_ptr<Component> &source, Button button, Modifiers modifiers, int x, int y, unsigned click_count, bool is_popup_trigger, const EventClock::time_point &when = EventClock::now()) :
      MouseEventBase(source, button, modifiers, x, y, when), click_count(click_count), is_popup_trigger(is_popup_trigger) {
  }
};

class MouseWheelEvent: public MouseEventBase {
public:
  int wheel_rotation;
public:
  constexpr MouseWheelEvent(const std::shared_ptr<Component> &source, Modifiers modifiers, int x, int y, int wheel_rotation, const EventClock::time_point &when = EventClock::now()) :
      MouseEventBase(source, NO_BUTTON, modifiers, x, y, when), wheel_rotation(wheel_rotation) {
  }
};

class MouseMoveEvent: public MouseEventBase {
public:
  constexpr MouseMoveEvent(const std::shared_ptr<Component> &source, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEventBase(source, NO_BUTTON, modifiers, x, y, when) {
  }
};

class MouseDragEvent: public MouseEventBase {
public:
  constexpr MouseDragEvent(const std::shared_ptr<Component> &source, Button button, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEventBase(source, button, modifiers, x, y, when) {
  }
};

}
