#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/lookandfeel/PanelUI.h>
#include <tui++/lookandfeel/FrameUI.h>
#include <tui++/lookandfeel/ButtonUI.h>
#include <tui++/lookandfeel/RootPaneUI.h>

namespace tui::laf {

std::shared_ptr<FrameUI> LookAndFeel::create_ui(Frame *c) {
  return {};
}

std::shared_ptr<PanelUI> LookAndFeel::create_ui(Panel *c) {
  return {};
}

std::shared_ptr<ButtonUI> LookAndFeel::create_ui(Button *c) {
  return {};
}

std::shared_ptr<MenuItemUI> LookAndFeel::create_ui(MenuItem *c) {
  return {};
}

std::shared_ptr<RootPaneUI> LookAndFeel::create_ui(RootPane *c) {
  return {};
}

std::shared_ptr<ToggleButtonUI> LookAndFeel::create_ui(ToggleButton *c) {
  return {};
}

}
