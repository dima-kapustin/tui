#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

namespace tui {
class Frame;
class Panel;
class Button;
class Dialog;
class MenuItem;
class RootPane;
class ToggleButton;
}

namespace tui::laf {

class FrameUI;
class PanelUI;
class ButtonUI;
class MenuItemUI;
class RootPaneUI;
class ToggleButtonUI;

class LookAndFeel {
public:
  static std::shared_ptr<FrameUI> create_ui(Frame *c);
  static std::shared_ptr<PanelUI> create_ui(Panel *c);
  static std::shared_ptr<ButtonUI> create_ui(Button *c);
  static std::shared_ptr<MenuItemUI> create_ui(MenuItem *c);
  static std::shared_ptr<RootPaneUI> create_ui(RootPane *c);
  static std::shared_ptr<ToggleButtonUI> create_ui(ToggleButton *c);
};

}
