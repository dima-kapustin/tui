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

public:
  bool was_button_down_before() const;

protected:
  template<typename Id>
  constexpr MouseEvent(const std::shared_ptr<Component> &source, const Id &id, Button button, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      InputEvent(source, id, modifiers, when), button(button), x(x), y(y) {
  }

  MouseEvent(MouseEvent const&) = default;
};

constexpr InputEvent::Modifiers to_modifiers(MouseEvent::Button button) {
  return button == MouseEvent::NO_BUTTON ? InputEvent::NO_MODIFIERS : InputEvent::Modifier(1 << (std::to_underlying(button) + 5));
}

static_assert(InputEvent::NO_MODIFIERS == to_modifiers(MouseEvent::NO_BUTTON));
static_assert(InputEvent::LEFT_BUTTON_DOWN == to_modifiers(MouseEvent::LEFT_BUTTON));
static_assert(InputEvent::MIDDLE_BUTTON_DOWN == to_modifiers(MouseEvent::MIDDLE_BUTTON));
static_assert(InputEvent::RIGHT_BUTTON_DOWN == to_modifiers(MouseEvent::RIGHT_BUTTON));

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

  constexpr Type type() const {
    return Type(std::underlying_type_t<Type>(this->id));
  }

protected:
  MousePressEvent(MousePressEvent const&) = default;
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

  constexpr Type type() const {
    return Type(std::underlying_type_t<Type>(this->id));
  }

protected:
  MouseOverEvent(MouseOverEvent const&) = default;
};

class MouseClickEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_CLICKED = event_id_v<EventType::MOUSE_CLICK>;
public:
  unsigned click_count;
  bool is_popup_trigger;

public:
  constexpr MouseClickEvent(const std::shared_ptr<Component> &source, Button button, Modifiers modifiers, int x, int y, unsigned click_count, bool is_popup_trigger, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, MOUSE_CLICKED, button, modifiers, x, y, when), click_count(click_count), is_popup_trigger(is_popup_trigger) {
  }

protected:
  MouseClickEvent(MouseClickEvent const&) = default;
};

class MouseWheelEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_WHEEL = event_id_v<EventType::MOUSE_WHEEL>;
public:
  int wheel_rotation;
public:
  constexpr MouseWheelEvent(const std::shared_ptr<Component> &source, Modifiers modifiers, int x, int y, int wheel_rotation, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, MOUSE_WHEEL, NO_BUTTON, modifiers, x, y, when), wheel_rotation(wheel_rotation) {
  }

protected:
  MouseWheelEvent(MouseWheelEvent const&) = default;
};

class MouseMoveEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_MOVED = event_id_v<EventType::MOUSE_MOVE>;
public:
  constexpr MouseMoveEvent(const std::shared_ptr<Component> &source, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, MOUSE_MOVED, NO_BUTTON, modifiers, x, y, when) {
  }

protected:
  MouseMoveEvent(MouseMoveEvent const&) = default;
};

class MouseDragEvent: public MouseEvent {
public:
  constexpr static unsigned MOUSE_DRAGGED = event_id_v<EventType::MOUSE_DRAG>;
public:
  constexpr MouseDragEvent(const std::shared_ptr<Component> &source, Button button, Modifiers modifiers, int x, int y, const EventClock::time_point &when = EventClock::now()) :
      MouseEvent(source, MOUSE_DRAGGED, button, modifiers, x, y, when) {
  }

protected:
  MouseDragEvent(MouseDragEvent const&) = default;
};

inline bool MouseEvent::was_button_down_before() const {
  constexpr auto BUTTON_DOWN_MASK = LEFT_BUTTON_DOWN | MIDDLE_BUTTON_DOWN | RIGHT_BUTTON_DOWN;

  auto modifiers = this->modifiers;
  if (this->id == MousePressEvent::MOUSE_PRESSED or this->id == MousePressEvent::MOUSE_RELEASED) {
    modifiers ^= to_modifiers(this->button);
  }
  /* modifiers now as just before event */
  return (modifiers & BUTTON_DOWN_MASK);
}

using MouseMovedListener = std::function<void(MouseMoveEvent &e)>;
using MouseOveredListener = std::function<void(MouseOverEvent &e)>;
using MouseClickedListener = std::function<void(MouseClickEvent &e)>;
using MouseDraggedListener = std::function<void(MouseDragEvent &e)>;
using MousePressedListener = std::function<void(MousePressEvent &e)>;
using MouseWheeledListener = std::function<void(MouseWheelEvent &e)>;

}
