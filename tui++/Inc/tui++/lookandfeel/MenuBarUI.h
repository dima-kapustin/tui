#pragma once

#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {
class MenuBar;
}

namespace tui::laf {

class MenuBarUI: public ComponentUI {
  MenuBar *menu_bar;

public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override;

protected:
  void install_defaults();
  void install_listeners();
  void install_keyboard_actions();
};

}

