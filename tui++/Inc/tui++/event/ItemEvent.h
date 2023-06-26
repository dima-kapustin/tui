#pragma once

#include <tui++/event/Event.h>

namespace tui {

class ItemEvent: public Event {
public:
  enum Type {
    SELECTED = event_id_v<EventType::ITEM, 0>,
    DESELECTED = event_id_v<EventType::ITEM, 1>
  };

public:
  constexpr ItemEvent(const std::shared_ptr<Component> &source, Type type) :
      Event(source, type) {
  }
};

}

