#pragma once

#include <tui++/event/Event.h>
#include <functional>

namespace tui {

class InvocationEvent: public Event {
  std::function<void()> target;

public:
  constexpr static unsigned INVOCATION = event_id_v<EventType::INVOCATION>;

public:
  InvocationEvent(const std::function<void()> &target) :
      Event(nullptr, EventType::INVOCATION), target(target) {
  }

  InvocationEvent(std::function<void()> &&target) :
      Event(nullptr, EventType::INVOCATION), target(std::move(target)) {
  }

  void dispatch() const {
    this->target();
  }
};

}
