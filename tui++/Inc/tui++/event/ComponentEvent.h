#pragma once

#include <tui++/event/Event.h>

#include <functional>

namespace tui {

class Component;

class ComponentEvent: public Event {
protected:
  template<typename Id>
  constexpr ComponentEvent(const std::shared_ptr<Component> &source, Id id, const EventClock::time_point &when = EventClock::now()) :
      ComponentEvent(source, std::to_underlying(id), when) {
  }

  ComponentEvent(const std::shared_ptr<Component> &source, unsigned id, const EventClock::time_point &when = EventClock::now());

  ComponentEvent(ComponentEvent const&) = default;

public:
  enum Type : unsigned {
    COMPONENT_SHOWN = event_id_v<EventType::COMPONENT, 0>,
    COMPONENT_HIDDEN = event_id_v<EventType::COMPONENT, 1> ,
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
