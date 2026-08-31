#pragma once

#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/terminal/text/TextTheme.h>

#include <tui++/lookandfeel/PanelUI.h>
#include <tui++/lookandfeel/FrameUI.h>
#include <tui++/lookandfeel/ButtonUI.h>
#include <tui++/lookandfeel/RootPaneUI.h>
#include <tui++/lookandfeel/ToggleButtonUI.h>

#include <tui++/lookandfeel/MenuUI.h>
#include <tui++/lookandfeel/MenuBarUI.h>
#include <tui++/lookandfeel/MenuItemUI.h>
#include <tui++/lookandfeel/PopupMenuUI.h>
#include <tui++/lookandfeel/SeparatorUI.h>
#include <tui++/lookandfeel/PopupMenuSeparatorUI.h>

namespace tui::laf {

// The default look-and-feel for the "text" (character-cell) terminal. Its UI
// delegates express sizes and positions in terminal cells.
class TextLookAndFeel: public LookAndFeel {
protected:
  std::shared_ptr<Theme> theme;

public:
  TextLookAndFeel() :
      theme(std::make_shared<TextTheme>()) {
    init_theme(this->theme);
  }

  virtual std::shared_ptr<Theme> get_theme() const override {
    return this->theme;
  }

  virtual std::shared_ptr<FrameUI> create_frame_ui(Frame *c) override {
    return std::make_shared<FrameUI>();
  }

  virtual std::shared_ptr<PanelUI> create_panel_ui(Panel *c) override {
    return std::make_shared<PanelUI>();
  }

  virtual std::shared_ptr<ButtonUI> create_button_ui(Button *c) override {
    return std::make_shared<ButtonUI>();
  }

  virtual std::shared_ptr<MenuItemUI> create_menu_item_ui(MenuItem *c) override {
    return std::make_shared<MenuItemUI>();
  }

  virtual std::shared_ptr<MenuUI> create_menu_ui(Menu *c) override {
    return std::make_shared<MenuUI>();
  }

  virtual std::shared_ptr<MenuBarUI> create_menu_bar_ui(MenuBar *c) override {
    return std::make_shared<MenuBarUI>();
  }

  virtual std::shared_ptr<RootPaneUI> create_root_pane_ui(RootPane *c) override {
    return std::make_shared<RootPaneUI>();
  }

  virtual std::shared_ptr<PopupMenuUI> create_popup_menu_ui(PopupMenu *c) override {
    return std::make_shared<PopupMenuUI>();
  }

  virtual std::shared_ptr<PopupMenuSeparatorUI> create_popup_menu_separator_ui(PopupMenuSeparator *c) override {
    return std::make_shared<PopupMenuSeparatorUI>();
  }

  virtual std::shared_ptr<SeparatorUI> create_separator_ui(Separator *c) override {
    return std::make_shared<SeparatorUI>();
  }

  virtual std::shared_ptr<ToggleButtonUI> create_toggle_button_ui(ToggleButton *c) override {
    return std::make_shared<ToggleButtonUI>();
  }
};

}
