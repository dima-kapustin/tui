#pragma once

#include <tui++/event/BasicEvent.h>

namespace tui {

class FocusEvent: public BasicEvent {
public:
  enum Type {
    FOCUS_LOST,
    FOCUS_GAINED
  };

public:
  const Type type;
  bool temporary;
  std::shared_ptr<Component> opposite;

public:
  FocusEvent(const std::shared_ptr<Component> &source, Type type, bool temporary, const std::shared_ptr<Component> &opposite) :
      BasicEvent(source), type(type), temporary(temporary), opposite(opposite) {
  }
};

}
