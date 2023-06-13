#pragma once

#include <tui++/event/BasicEvent.h>

namespace tui {

class ItemEvent: public BasicEvent {
public:
  enum Type {
    SELECTED,
    DESELECTED
  };
};

}

