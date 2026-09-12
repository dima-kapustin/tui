#include <tui++/Switch.h>

#include <tui++/lookandfeel/SwitchUI.h>
#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {

std::shared_ptr<laf::SwitchUI> Switch::get_ui() const {
  return std::static_pointer_cast<laf::SwitchUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> Switch::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

}
