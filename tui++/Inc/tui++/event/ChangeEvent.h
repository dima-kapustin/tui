#pragma once

#include <functional>

namespace tui {

class Object;

struct ChangeEvent {
 const std::shared_ptr<Object> source;

  constexpr ChangeEvent(const std::shared_ptr<Object> &source) :
      source(source) {
  }
};

using ChangeListener = std::function<void(ChangeEvent &e)>;

}
