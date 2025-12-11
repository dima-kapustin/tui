#pragma once

#include <memory>

#include <tui++/event/EventId.h>

namespace tui {

class Screen;
class Component;
class KeyboardFocusManager;

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
  constexpr Event(const std::shared_ptr<Component> &source, const Id &id) :
      id(id), source(source) {
  }

  Event(const Event&) = delete;
  Event(Event&&) = delete;
  Event& operator=(const Event&) = delete;
  Event& operator=(Event&&) = delete;

public:
  virtual ~Event() {
  }

public:
  const EventId id;
  std::shared_ptr<Component> source;
  bool consumed = false;
};

}
