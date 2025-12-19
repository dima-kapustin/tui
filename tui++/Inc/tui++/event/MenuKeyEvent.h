#pragma once

#include <tui++/event/KeyEvent.h>

#include <memory>

namespace tui {

class MenuElement;
class MenuSelectionManager;

class MenuKeyEvent: public KeyEvent {
public:
  MenuKeyEvent(KeyEvent const &key_event, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) :
      KeyEvent(key_event), path(path), manager(manager) {
  }

  std::vector<std::shared_ptr<MenuElement>> const path;
  std::shared_ptr<MenuSelectionManager> const manager;
};

}
