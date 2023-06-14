#pragma once

#include <tui++/event/BasicEvent.h>

namespace tui {

class ItemEvent: public BasicEvent {
public:
  enum StateChange {
    SELECTED,
    DESELECTED
  };

  const StateChange state_change;

public:
  constexpr ItemEvent(const std::shared_ptr<Component> &source, StateChange state_change) :
      BasicEvent(source), state_change(state_change) {
  }
};

}

