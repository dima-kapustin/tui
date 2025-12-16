#pragma once

#include <tui++/event/Event.h>

#include <functional>

namespace tui {

class Object;

class ItemEvent: public Event {
public:
  enum Type : unsigned {
    SELECTED = event_id_v<EventType::ITEM, 0>,
    DESELECTED = event_id_v<EventType::ITEM, 1>
  };

public:
  constexpr ItemEvent(const std::shared_ptr<Object> &source, Type type) :
      ItemEvent(source, type, source) {
  }

  constexpr ItemEvent(const std::shared_ptr<Object> &source, Type type, const std::shared_ptr<Object> &item) :
      Event(source, type) {
  }

  constexpr Type type() const {
    return Type(std::underlying_type_t<Type>(this->id));
  }

  const std::shared_ptr<Object> item;
};

using ItemEventListener = std::function<void(ItemEvent &e)>;

}

