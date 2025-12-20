#include <tui++/Separator.h>

#include <tui++/lookandfeel/SeparatorUI.h>

namespace tui {
std::shared_ptr<laf::SeparatorUI> Separator::get_ui() const {
  return std::static_pointer_cast<laf::SeparatorUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> Separator::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}
}
