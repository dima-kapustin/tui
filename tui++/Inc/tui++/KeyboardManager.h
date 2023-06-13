#pragma once

#include <memory>

namespace tui {

class KeyEvent;
class Component;

class KeyboardManager {
public:
  static bool fire_keyboard_action(KeyEvent &e, const std::shared_ptr<Component> &top_ancestor);
};

}

