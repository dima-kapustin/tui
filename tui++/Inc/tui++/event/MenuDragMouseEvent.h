#pragma once

#include <tui++/event/MouseEvent.h>

#include <memory>

namespace tui {

class MenuElement;
class MenuSelectionManager;

namespace detail {
class MenuDragMouseEventBase {
public:
  MenuDragMouseEventBase(std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) :
    path(path), manager(manager) {
  }

  std::vector<std::shared_ptr<MenuElement>> const path;
  std::shared_ptr<MenuSelectionManager> const manager;
};
}

template<typename MouseEvent>
requires (std::is_base_of_v<MousePressEvent, MouseEvent> or std::is_base_of_v<MouseDragEvent, MouseEvent> or std::is_base_of_v<MouseOverEvent, MouseEvent>)
class MenuDragMouseEvent;

template<>
class MenuDragMouseEvent<MousePressEvent>: protected MousePressEvent, public detail::MenuDragMouseEventBase {
public:
  MenuDragMouseEvent(MousePressEvent const &mouse_event, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) :
    MousePressEvent(mouse_event), detail::MenuDragMouseEventBase(path, manager) {
  }
};

template<>
class MenuDragMouseEvent<MouseOverEvent>: protected MouseOverEvent, public detail::MenuDragMouseEventBase {
public:
  MenuDragMouseEvent(MouseOverEvent const &mouse_event, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) :
    MouseOverEvent(mouse_event), detail::MenuDragMouseEventBase(path, manager) {
  }
};

template<>
class MenuDragMouseEvent<MouseDragEvent>: protected MouseDragEvent, public detail::MenuDragMouseEventBase {
public:
  MenuDragMouseEvent(MouseDragEvent const &mouse_event, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) :
    MouseDragEvent(mouse_event), detail::MenuDragMouseEventBase(path, manager) {
  }
};

using MenuDragMouseOveredListener = std::function<void(MenuDragMouseEvent<MouseOverEvent> &e)>;
using MenuDragMouseDraggedListener = std::function<void(MenuDragMouseEvent<MouseDragEvent> &e)>;
using MenuDragMousePressedListener = std::function<void(MenuDragMouseEvent<MousePressEvent> &e)>;

}
