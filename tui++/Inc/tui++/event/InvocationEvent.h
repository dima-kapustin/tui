#pragma once

#include <functional>

#include <tui++/event/BasicEvent.h>

namespace tui {

class InvocationEvent: public BasicEvent {
  std::function<void()> target;

public:
  InvocationEvent(const std::function<void()> &target) :
      target(target) {
  }

  void dispatch() const {
    this->target();
  }
};

}
