#pragma once

#include <tui++/event/Event.h>

namespace tui {

class Component;

class ComponentEvent: public Event {
protected:
  template<typename Id>
  constexpr ComponentEvent(const std::shared_ptr<Component> &source, const Id &id);

public:
  enum Type : unsigned {
    COMPONENT_SHOWN = event_id_v<EventType::COMPONENT, 0>,
    COMPONENT_HIDDEN = event_id_v<EventType::COMPONENT, 1> ,
  };

public:
  constexpr ComponentEvent(const std::shared_ptr<Component> &source, Type type);

  std::shared_ptr<Component> component() const;
};

}
