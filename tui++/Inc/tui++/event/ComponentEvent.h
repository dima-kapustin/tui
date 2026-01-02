#pragma once

#include <tui++/event/Event.h>

#include <functional>

namespace tui {

class Component;

class ComponentEvent: public Event {
protected:
  ComponentEvent(const std::shared_ptr<Component> &source, unsigned id, const EventClock::time_point &when = EventClock::now());

  ComponentEvent(ComponentEvent const&) = default;

public:
  enum Type : unsigned {
    COMPONENT_MOVED = event_id_v<EventType::COMPONENT, 0>,
    COMPONENT_RESIZED = event_id_v<EventType::COMPONENT, 1> ,
    COMPONENT_SHOWN = event_id_v<EventType::COMPONENT, 2>,
    COMPONENT_HIDDEN = event_id_v<EventType::COMPONENT, 3> ,
  };

public:
  ComponentEvent(const std::shared_ptr<Component> &source, Type type, const EventClock::time_point &when = EventClock::now());

  constexpr Type type() const {
    return Type(std::underlying_type_t<Type>(this->id));
  }

  std::shared_ptr<Component> component() const;
};

using ComponentListener = std::function<void(ComponentEvent &e)>;

}
