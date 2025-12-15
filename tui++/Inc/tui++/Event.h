#pragma once

#include <tui++/event/KeyEvent.h>
#include <tui++/event/ItemEvent.h>
#include <tui++/event/FocusEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/event/ActionEvent.h>
#include <tui++/event/WindowEvent.h>
#include <tui++/event/ComponentEvent.h>
#include <tui++/event/ContainerEvent.h>
#include <tui++/event/HierarchyEvent.h>
#include <tui++/event/HierarchyBoundsEvent.h>
#include <tui++/event/InvocationEvent.h>

#include <tui++/event/EventListener.h>

#include <variant>

namespace tui {

template<typename Event>
struct event_type;

template<>
struct event_type<ActionEvent> {
  constexpr static EventType value = EventType::ACTION;
};

template<>
struct event_type<ComponentEvent> {
  constexpr static EventType value = EventType::COMPONENT;
};

template<>
struct event_type<ContainerEvent> {
  constexpr static EventType value = EventType::CONTAINER;
};

template<>
struct event_type<FocusEvent> {
  constexpr static EventType value = EventType::FOCUS;
};

template<>
struct event_type<HierarchyEvent> {
  constexpr static EventType value = EventType::HIERARCHY;
};

template<>
struct event_type<HierarchyBoundsEvent> {
  constexpr static EventType value = EventType::HIERARCHY_BOUNDS;
};

template<>
struct event_type<InvocationEvent> {
  constexpr static EventType value = EventType::INVOCATION;
};

template<>
struct event_type<ItemEvent> {
  constexpr static EventType value = EventType::ITEM;
};

template<>
struct event_type<KeyEvent> {
  constexpr static EventType value = EventType::KEY;
};

template<>
struct event_type<MousePressEvent> {
  constexpr static EventType value = EventType::MOUSE_PRESS;
};

template<>
struct event_type<MouseClickEvent> {
  constexpr static EventType value = EventType::MOUSE_CLICK;
};

template<>
struct event_type<MouseMoveEvent> {
  constexpr static EventType value = EventType::MOUSE_MOVE;
};

template<>
struct event_type<MouseDragEvent> {
  constexpr static EventType value = EventType::MOUSE_DRAG;
};

template<>
struct event_type<MouseOverEvent> {
  constexpr static EventType value = EventType::MOUSE_OVER;
};

template<>
struct event_type<MouseWheelEvent> {
  constexpr static EventType value = EventType::MOUSE_WHEEL;
};

template<>
struct event_type<WindowEvent> {
  constexpr static EventType value = EventType::WINDOW;
};

template<typename Event>
constexpr EventType event_type_v = event_type<Event>::value;

template<typename Event, typename ... Events>
constexpr EventTypeMask event_mask_v = (event_type_v<Event> | ... | event_type_v<Events>);

constexpr EventTypeMask MOUSE_EVENT_MASK = EventType::MOUSE_PRESS | EventType::MOUSE_CLICK | EventType::MOUSE_DRAG | EventType::MOUSE_MOVE | EventType::MOUSE_OVER | EventType::MOUSE_WHEEL;
constexpr EventTypeMask KEY_EVENT_MASK = EventType::KEY;
constexpr EventTypeMask WINDOW_EVENT_MASK = EventType::WINDOW;

template<typename E>
struct is_mouse_event {
  constexpr static bool value = MOUSE_EVENT_MASK & event_type_v<E>;
};

template<typename E>
constexpr bool is_mouse_event_v = is_mouse_event<E>::value;

std::ostream& operator<<(std::ostream &os, const Event &event);

template<>
class EventListener<Event> : public BasicEventListener<Event> {
public:
  virtual void event_dispatched(Event &e) = 0;
};

template<typename T, typename ...Args>
constexpr auto make_event(Args &&... args) {
  return T { std::forward<Args>(args)... };
}

}
