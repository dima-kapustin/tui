#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/lookandfeel/PanelUI.h>
#include <tui++/lookandfeel/FrameUI.h>
#include <tui++/lookandfeel/ButtonUI.h>
#include <tui++/lookandfeel/RootPaneUI.h>
#include <tui++/lookandfeel/PopupMenuUI.h>

#include <tui++/lookandfeel/MenuUI.h>
#include <tui++/lookandfeel/MenuBarUI.h>
#include <tui++/lookandfeel/MenuItemUI.h>

namespace tui::laf {

static class TuiTheme {
public:
  TuiTheme() {
    LookAndFeel::put( //
        { { "menu.submenu-popup-offset-x", 0 }, //
          { "menu.submenu-popup-offset-y", 0 }, //
          { "menu.menu-popup-offset-x", 0 }, //
          { "menu.menu-popup-offset-y", 0 }, //
        });
  }
} tui_theme;

void LookAndFeel::put(std::initializer_list<std::pair<std::string_view, std::any>> &&values) {
  for (auto&& [key, value] : values) {
    properties.emplace(std::move(key), std::move(value));
  }
}

std::shared_ptr<FrameUI> LookAndFeel::create_ui(Frame *c) {
  return {};
}

std::shared_ptr<PanelUI> LookAndFeel::create_ui(Panel *c) {
  return {};
}

std::shared_ptr<ButtonUI> LookAndFeel::create_ui(Button *c) {
  return {};
}

std::shared_ptr<MenuUI> LookAndFeel::create_ui(Menu *c) {
  return {};
}

std::shared_ptr<MenuBarUI> LookAndFeel::create_ui(MenuBar *c) {
  return {};
}

std::shared_ptr<MenuItemUI> LookAndFeel::create_ui(MenuItem *c) {
  return {};
}

std::shared_ptr<RootPaneUI> LookAndFeel::create_ui(RootPane *c) {
  return {};
}

std::shared_ptr<PopupMenuUI> LookAndFeel::create_ui(PopupMenu *c) {
  return {};
}

std::shared_ptr<ToggleButtonUI> LookAndFeel::create_ui(ToggleButton *c) {
  return {};
}

}
