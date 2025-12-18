#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

namespace tui {
class Frame;
class Panel;
class Button;
class Dialog;
class RootPane;
}

namespace tui::laf {

class FrameUI;
class PanelUI;
class ButtonUI;
class RootPaneUI;

class LookAndFeel {
public:
  static std::shared_ptr<FrameUI> create_ui(Frame *c);
  static std::shared_ptr<PanelUI> create_ui(Panel *c);
  static std::shared_ptr<ButtonUI> create_ui(Button *c);
  static std::shared_ptr<RootPaneUI> create_ui(RootPane *c);
};

}
