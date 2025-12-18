#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/lookandfeel/PanelUI.h>
#include <tui++/lookandfeel/FrameUI.h>
#include <tui++/lookandfeel/ButtonUI.h>
#include <tui++/lookandfeel/RootPaneUI.h>

namespace tui::laf {

  std::shared_ptr<RootPaneUI> LookAndFeel::get_ui(RootPane *c) {
  return {};
}

std::shared_ptr<FrameUI> LookAndFeel::get_ui(Frame *c) {
  return {};
}

std::shared_ptr<PanelUI> LookAndFeel::get_ui(Panel *c) {
  return {};
}

std::shared_ptr<ButtonUI> LookAndFeel::get_ui(Button *c) {
  return {};
}

}
