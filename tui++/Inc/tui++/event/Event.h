#pragma once

#include <memory>
#include <chrono>

#include <tui++/event/EventId.h>

namespace tui {

class Object;
class Screen;
class KeyboardFocusManager;

using EventClock = std::chrono::utc_clock;

class Event {
  struct {
    unsigned system_generated :1 = false;
    unsigned is_posted :1 = false;
    unsigned is_being_dispatched_by_focus_manager :1 = false;
  };

  friend class Screen;
  friend class Component;
  friend class KeyboardFocusManager;

protected:
  template<typename Id>
  constexpr Event(const std::shared_ptr<Object> &source, Id id, EventClock::time_point const &when = EventClock::now()) :
      id(id), source(source), when(when) {
  }

  Event(const Event&) = default;
  Event(Event&&) = delete;
  Event& operator=(const Event&) = delete;
  Event& operator=(Event&&) = delete;

public:
  virtual ~Event() {
  }

  void consume() {
    this->consumed = true;
  }

public:
  const EventId id;
  std::shared_ptr<Object> source;
  bool consumed = false;
  EventClock::time_point when;
};

}
