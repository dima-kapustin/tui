#pragma once

#include <tui++/event/Event.h>

#include <functional>

namespace tui {

class Object;

class ItemEvent: public Event {
public:
  enum Type {
    SELECTED = event_id_v<EventType::ITEM, 0>,
    DESELECTED = event_id_v<EventType::ITEM, 1>
  };

public:
  constexpr ItemEvent(const std::shared_ptr<Object> &source, Type type) :
      Event(source, type) {
  }
};


using ItemListener = std::function<void(ItemEvent &e)>;

}

