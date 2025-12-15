#pragma once

#include <tui++/event/InputEvent.h>
#include <tui++/Point.h>

namespace tui {

class MouseEvent: public InputEvent {
public:
  enum Button {
    LEFT_BUTTON = 0,
    MIDDLE_BUTTON = 1,
    RIGHT_BUTTON = 2,
    NO_BUTTON = 3
  };

public:
  Button button = NO_BUTTON;
  union {
    struct {
      int x, y;
    };
    Point point;
  };

protected:
  template<typename Id>
  constexpr MouseEvent(const std::shared_ptr<Component> &source, const Id &id, Button button, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      InputEvent(source, id, modifiers, when), button(button), x(x), y(y) {
  }

public:
  bool was_button_down_before() const;
};

constexpr InputEvent::Modifier to_modifier(MouseEvent::Button button) {
  return button == MouseEvent::NO_BUTTON ? InputEvent::NO_MODIFIERS : InputEvent::Modifier(1 << (std::to_underlying(button) + 5));
}

static_assert(InputEvent::Modifier::NO_MODIFIERS == to_modifier(MouseEvent::NO_BUTTON));
static_assert(InputEvent::Modifier::LEFT_BUTTON_DOWN == to_modifier(MouseEvent::LEFT_BUTTON));
static_assert(InputEvent::Modifier::MIDDLE_BUTTON_DOWN == to_modifier(MouseEvent::MIDDLE_BUTTON));
static_assert(InputEvent::Modifier::RIGHT_BUTTON_DOWN == to_modifier(MouseEvent::RIGHT_BUTTON));

class MousePressEvent: public MouseEvent {
public:
  enum Type {
    MOUSE_PRESSED = event_id_v<EventType::MOUSE_PRESS, 0>,
    MOUSE_RELEASED = event_id_v<EventType::MOUSE_PRESS, 1>
  };

public:
  bool is_popup_trigger;

public:
  constexpr MousePressEvent(const std::shared_ptr<Component> &source, Type type, Button button, Modifiers modifiers, int x, int y, bool is_popup_trigger, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, type, button, modifiers, x, y, when), is_popup_trigger(is_popup_trigger) {
  }
};

class MouseOverEvent: public MouseEvent {
public:
  enum Type {
    MOUSE_ENTERED = event_id_v<EventType::MOUSE_OVER, 0>,
    MOUSE_EXITED = event_id_v<EventType::MOUSE_OVER, 1> ,
  };

public:
  constexpr MouseOverEvent(const std::shared_ptr<Component> &source, Type type, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, type, MouseEvent::NO_BUTTON, modifiers, x, y, when) {
  }
};

class MouseClickEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_CLICKED = event_id_v<EventType::MOUSE_CLICK>;
public:
  unsigned click_count;
  bool is_popup_trigger;

public:
  constexpr MouseClickEvent(const std::shared_ptr<Component> &source, Button button, Modifiers modifiers, int x, int y, unsigned click_count, bool is_popup_trigger, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, EventType::MOUSE_CLICK, button, modifiers, x, y, when), click_count(click_count), is_popup_trigger(is_popup_trigger) {
  }
};

class MouseWheelEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_WHEEL = event_id_v<EventType::MOUSE_WHEEL>;
public:
  int wheel_rotation;
public:
  constexpr MouseWheelEvent(const std::shared_ptr<Component> &source, Modifiers modifiers, int x, int y, int wheel_rotation, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, EventType::MOUSE_WHEEL, NO_BUTTON, modifiers, x, y, when), wheel_rotation(wheel_rotation) {
  }
};

class MouseMoveEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_MOVED = event_id_v<EventType::MOUSE_MOVE>;
public:
  constexpr MouseMoveEvent(const std::shared_ptr<Component> &source, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, EventType::MOUSE_MOVE, NO_BUTTON, modifiers, x, y, when) {
  }
};

class MouseDragEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_DRAGGED = event_id_v<EventType::MOUSE_DRAG>;
public:
  constexpr MouseDragEvent(const std::shared_ptr<Component> &source, Button button, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, EventType::MOUSE_DRAG, button, modifiers, x, y, when) {
  }
};

inline bool MouseEvent::was_button_down_before() const {
  constexpr auto BUTTON_DOWN_MASK = LEFT_BUTTON_DOWN | MIDDLE_BUTTON_DOWN | RIGHT_BUTTON_DOWN;

  auto modifiers = this->modifiers;
  if (this->id == MousePressEvent::MOUSE_PRESSED or this->id == MousePressEvent::MOUSE_RELEASED) {
    modifiers ^= to_modifier(this->button);
  }
  /* modifiers now as just before event */
  return (modifiers & BUTTON_DOWN_MASK);
}

}
