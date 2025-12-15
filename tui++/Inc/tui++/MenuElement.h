#pragma once

#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>

#include <memory>
#include <vector>

namespace tui {

class Component;
class MenuSelectionManager;

struct MenuElement {
  virtual ~MenuElement() {
  }

public:
  virtual void process_mouse_event(MouseEventBase &e, std::vector<std::shared_ptr<MenuElement>> const &path, MenuSelectionManager &manager) = 0;

  virtual void process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, MenuSelectionManager &manager) = 0;

  virtual void menu_selection_changed(bool is_included) = 0;

  virtual std::vector<std::shared_ptr<MenuElement>> get_sub_elements() = 0;

  virtual std::shared_ptr<Component> get_component() = 0;
};

}
