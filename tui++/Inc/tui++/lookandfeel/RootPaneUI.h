#pragma once

#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {
class RootPane;
}

namespace tui::laf {

class LazyActionMap;

// Swing's BasicRootPaneUI. It installs the keyboard behavior that belongs to
// the window as a whole rather than to any one component: the default button's
// Enter.
class RootPaneUI: public ComponentUI {
  RootPane *root_pane = nullptr;

public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override;

  virtual void uninstall_ui(std::shared_ptr<Component> const &c) override;

protected:
  virtual void install_keyboard_actions();

  static void load_action_map(LazyActionMap &map);
};

}
