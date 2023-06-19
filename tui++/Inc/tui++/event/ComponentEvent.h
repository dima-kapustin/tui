#pragma once

#include <tui++/event/BasicEvent.h>

namespace tui {

class ComponentEvent: public BasicEvent {
public:
  enum Type {
    COMPONENT_SHOWN
  };

public:
  const Type type;

public:
  ComponentEvent(const std::shared_ptr<Component> &source, Type type) :
      BasicEvent(source), type(type) {
  }
};

}
