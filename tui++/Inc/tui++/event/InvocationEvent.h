#pragma once

#include <functional>

namespace tui {

class InvocationEvent {
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
