#pragma once

#include <tui++/InputMap.h>

namespace tui {

class Component;

class ComponentInputMap: public InputMap {
  std::weak_ptr<Component> component;

public:
  ComponentInputMap(const std::shared_ptr<Component> &component) :
      component(component) {
  }
};

}
