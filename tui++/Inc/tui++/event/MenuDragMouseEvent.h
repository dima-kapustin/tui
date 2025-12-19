#pragma once

#include <tui++/event/MouseEvent.h>

#include <memory>

namespace tui {

class MenuElement;
class MenuSelectionManager;

template<typename MouseEvent>
requires (std::is_base_of_v<MousePressEvent, MouseEvent> or std::is_base_of_v<MouseDragEvent, MouseEvent> or std::is_base_of_v<MouseOverEvent, MouseEvent>)
class MenuDragMouseEvent: public MouseEvent {
public:
  MenuDragMouseEvent(MouseEvent const &mouse_event, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) :
    MouseEvent(mouse_event), path(path), manager(manager) {
  }

  std::vector<std::shared_ptr<MenuElement>> const path;
  std::shared_ptr<MenuSelectionManager> const manager;
};

}
