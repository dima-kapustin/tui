#include <tui++/MenuBar.h>

#include <tui++/lookandfeel/MenuBarUI.h>

namespace tui {

std::shared_ptr<laf::MenuBarUI> MenuBar::get_ui() const {
  return std::static_pointer_cast<laf::MenuBarUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> MenuBar::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

}
