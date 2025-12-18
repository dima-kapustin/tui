#pragma once

#include <tui++/event/ComponentEvent.h>

namespace tui {

class ContainerEvent: public ComponentEvent {
public:
  enum Type : unsigned {
    COMPONENT_ADDED = event_id_v<EventType::CONTAINER, 0>,
    COMPONENT_REMOVED = event_id_v<EventType::CONTAINER, 1>
  };
public:
  ContainerEvent(const std::shared_ptr<Component> &source, Type type, const std::shared_ptr<Component> &child) :
      ComponentEvent(source, type) {
  }

  constexpr Type type() const {
    return Type(std::underlying_type_t<Type>(this->id));
  }
};

using ContainerListener = std::function<void(ContainerEvent &e)>;

}
