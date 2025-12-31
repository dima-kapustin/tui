#pragma once

#include <tui++/InputMap.h>

namespace tui {

class Component;

class ComponentInputMap: public InputMap {
  Component *component;

public:
  ComponentInputMap(Component *component) :
      component(component) {
  }
};

}
