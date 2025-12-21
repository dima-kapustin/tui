#pragma once

#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {
class Popup;
}

namespace tui::laf {

class PopupMenuUI: public ComponentUI {
public:
  std::shared_ptr<Popup> get_popup(std::shared_ptr<PopupMenu> const& menu, int x, int y);
};

}

