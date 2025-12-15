#pragma once

#include <functional>

namespace tui {

class Object;

struct ChangeEvent {
  Object *const source;

  constexpr ChangeEvent(Object *source) :
      source(source) {
  }
};

using ChangeListener = std::function<void(ChangeEvent &e)>;

}
