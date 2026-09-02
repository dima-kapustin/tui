#pragma once

#include <tui++/PopupMenu.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/lookandfeel/MenuLayout.h>

namespace tui {
class Popup;
}

namespace tui::laf {

class PopupMenuUI: public ComponentUI {
  PopupMenu *popup_menu;

public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override {
    ComponentUI::install_ui(c);
    this->popup_menu = static_cast<PopupMenu*>(c.get());
    install_defaults();
  }

  virtual void uninstall_ui(std::shared_ptr<Component> const &c) override {
    this->popup_menu = nullptr;
    ComponentUI::uninstall_ui(c);
  }

  std::shared_ptr<Popup> get_popup(std::shared_ptr<PopupMenu> const& menu, int x, int y);

protected:
  void install_defaults() {
    // As in Swing's BasicPopupMenuUI: a vertical menu layout so the items
    // drop down in a column (without it the popup sizes itself 1x1 and the
    // pack in PopupWindow::show leaves it invisible), an opaque background,
    // and the theme's popup colors/border where the theme defines them.
    if (not this->popup_menu->get_layout()) {
      this->popup_menu->set_layout(std::make_shared<MenuLayout>(this->popup_menu, MenuLayout::Y));
    }
    this->popup_menu->set_opaque(true);
    LookAndFeel::install_colors(this->popup_menu, "PopupMenu.BackgroundColor", "PopupMenu.ForegroundColor");
    LookAndFeel::install_border(this->popup_menu, "PopupMenu.Border");
  }
};

}
