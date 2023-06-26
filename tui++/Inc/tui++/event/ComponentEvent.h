#pragma once

#include <tui++/event/Event.h>

namespace tui {

class ComponentEvent: public Event {
public:
  enum Type : unsigned {
    COMPONENT_SHOWN = event_id_v<EventType::COMPONENT, 0>,
    COMPONENT_HIDDEN = event_id_v<EventType::COMPONENT, 1>,
  };

public:
  ComponentEvent(const std::shared_ptr<Component> &source, Type type) :
      Event(source, type) {
  }
};

}
