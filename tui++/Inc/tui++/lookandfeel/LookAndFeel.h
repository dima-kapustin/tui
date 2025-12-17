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

class FrameUI: public ComponentUI {
};

class RootPaneUI: public ComponentUI {
};

class PanelUI: public ComponentUI {
};

class LookAndFeel {
public:
  static std::shared_ptr<RootPaneUI> get_ui(RootPane *c) {
    return {};
  }

  static std::shared_ptr<FrameUI> get_ui(Frame *c) {
    return {};
  }

  static std::shared_ptr<PanelUI> get_ui(Panel *c) {
    return {};
  }
};

}
