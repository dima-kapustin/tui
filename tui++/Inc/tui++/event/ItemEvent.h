#pragma once

#include <tui++/event/BasicEvent.h>

namespace tui {

class ItemEvent: public BasicEvent {
public:
  enum Type {
    SELECTED,
    DESELECTED
  };

  const Type type;

public:
  constexpr ItemEvent(const std::shared_ptr<Component> &source, Type type) :
      BasicEvent(source), type(type) {
  }
};

}

