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
  static std::shared_ptr<FrameUI> get_ui(Frame *c);
  static std::shared_ptr<PanelUI> get_ui(Panel *c);
  static std::shared_ptr<ButtonUI> get_ui(Button *c);
  static std::shared_ptr<RootPaneUI> get_ui(RootPane *c);
};

}
