#pragma once

#include <memory>

namespace tui {

class Component;

class BasicEvent {
protected:
  constexpr BasicEvent() = default;
  constexpr BasicEvent(const std::shared_ptr<Component> &source) :
      source(source) {
  }

public:
  std::shared_ptr<Component> source;
};

}
