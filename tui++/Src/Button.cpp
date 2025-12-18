#include <tui++/Button.h>

#include <tui++/lookandfeel/ButtonUI.h>

namespace tui {
std::shared_ptr<laf::ButtonUI> Button::get_ui() const {
  return std::static_pointer_cast<laf::ButtonUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> Button::create_ui() {
  return laf::LookAndFeel::get_ui(this);
}

}
