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
#include <tui++/event/InvocationEvent.h>

#include <tui++/util/EnumFlags.h>

#include <variant>

namespace tui {

using EventVariant = std::variant<
/**/std::monostate,
/**/KeyEvent,
/**/ItemEvent,
/**/FocusEvent,
/**/MouseEvent,
/**/MouseMoveEvent,
/**/MouseDragEvent,
/**/MouseClickEvent,
/**/MouseWheelEvent,
/**/ActionEvent,
/**/WindowEvent,
/**/ComponentEvent,
/**/ContainerEvent,
/**/HierarchyEvent,
/**/InvocationEvent>;

class Screen;
class KeyboardFocusManager;

class Event: public EventVariant {
  unsigned system_generated :1 = false;
  unsigned is_posted :1 = false;
  unsigned focus_manager_is_dispatching :1 = false;

  friend class Screen;
  friend class Component;
  friend class KeyboardFocusManager;

public:
  Event() = default;

  template<typename T, typename ...Args>
  Event(std::in_place_type_t<T>, Args ... args) :
      EventVariant(std::in_place_type<T>, std::forward<Args>(args)...) {
  }
};

std::ostream& operator<<(std::ostream &os, const Event &event);

enum class EventType {
  UNDEFINED = 0,
  KEY = 1 << 0,
  ITEM = 1 << 1,
  FOCUS = 1 << 2,
  MOUSE = 1 << 3,
  MOUSE_MOVE = 1 << 4,
  MOUSE_DRAG = 1 << 5,
  MOUSE_CLICK = 1 << 6,
  MOUSE_WHEEL = 1 << 7,
  ACTION = 1 << 8,
  WINDOW = 1 << 9,
  COMPONENT = 1 << 10,
  CONTAINER = 1 << 11,
  HIERARCHY = 1 << 12,
  INVOCATION = 1 << 13,
};

template<EventType event_type>
struct event_alternative: std::variant_alternative<std::countr_zero(std::make_unsigned_t<std::underlying_type_t<EventType>>(std::to_underlying(event_type))) + 1, EventVariant> {
};

template<>
struct event_alternative<EventType::UNDEFINED> : std::variant_alternative<std::to_underlying(EventType::UNDEFINED), EventVariant> {
};

template<EventType event_type>
using event_alternative_t = typename event_alternative<event_type>::type;

static_assert(std::is_same_v<std::monostate, event_alternative_t<EventType::UNDEFINED>>);
static_assert(std::is_same_v<KeyEvent, event_alternative_t<EventType::KEY>>);
static_assert(std::is_same_v<ItemEvent, event_alternative_t<EventType::ITEM>>);
static_assert(std::is_same_v<FocusEvent, event_alternative_t<EventType::FOCUS>>);
static_assert(std::is_same_v<MouseEvent, event_alternative_t<EventType::MOUSE>>);
static_assert(std::is_same_v<MouseMoveEvent, event_alternative_t<EventType::MOUSE_MOVE>>);
static_assert(std::is_same_v<MouseDragEvent, event_alternative_t<EventType::MOUSE_DRAG>>);
static_assert(std::is_same_v<MouseClickEvent, event_alternative_t<EventType::MOUSE_CLICK>>);
static_assert(std::is_same_v<MouseWheelEvent, event_alternative_t<EventType::MOUSE_WHEEL>>);
static_assert(std::is_same_v<ActionEvent, event_alternative_t<EventType::ACTION>>);
static_assert(std::is_same_v<WindowEvent, event_alternative_t<EventType::WINDOW>>);
static_assert(std::is_same_v<ComponentEvent, event_alternative_t<EventType::COMPONENT>>);
static_assert(std::is_same_v<ContainerEvent, event_alternative_t<EventType::CONTAINER>>);
static_assert(std::is_same_v<HierarchyEvent, event_alternative_t<EventType::HIERARCHY>>);
static_assert(std::is_same_v<InvocationEvent, event_alternative_t<EventType::INVOCATION>>);

using EventTypeMask = util::EnumFlags<EventType>;

namespace detail {

template<typename T, typename V>
struct variant_alternative_index;

template<typename T, typename ... Ts>
struct variant_alternative_index<T, std::variant<Ts...>> : std::integral_constant<size_t, std::variant<std::type_identity<Ts>...>(std::type_identity<T>()).index()> {
};

template<typename T, typename ... Ts>
constexpr size_t variant_alternative_index_v = variant_alternative_index<T, Ts...>::value;

}

template<typename Event>
constexpr EventTypeMask to_event_mask() {
  constexpr auto index = detail::variant_alternative_index_v<Event, EventVariant>;
  if constexpr (index == 0) {
    static_assert(std::is_same_v<Event, std::monostate>);
    static_assert(std::is_same_v<std::monostate, event_alternative_t<EventType(index)>>);
    return EventType::UNDEFINED;
  } else {
    static_assert(std::is_same_v<Event, event_alternative_t<EventType(1 << (index - 1))>>);
    return EventType(1 << (index - 1));
  }
}

}
