#pragma once

#include <tui++/MenuElement.h>
#include <tui++/AbstractButton.h>

namespace tui {

class MenuItem: public AbstractButton, public MenuElement {


public:
  void process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  void process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  void menu_selection_changed(bool is_included) override;

  std::vector<std::shared_ptr<MenuElement>> get_sub_elements() override;

  std::shared_ptr<Component> get_component() override;
};

}
