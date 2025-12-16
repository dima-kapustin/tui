#pragma once

#include <memory>

namespace tui {

class Component;

class ToolTipManager {
public:
  static void register_component(std::shared_ptr<Component> const &c) {

  }

  static void unregister_component(std::shared_ptr<Component> const &c) {

  }
};

}
