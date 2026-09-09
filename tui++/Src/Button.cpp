#include <tui++/Button.h>
#include <tui++/RootPane.h>

#include <tui++/lookandfeel/ButtonUI.h>
#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {
Button::Button(std::string const &text) :
    Button(text, Char { }) {
}

Button::Button(std::string const &text, Char const &mnemonic) :
    AbstractButton(std::make_shared<ButtonModel>(), text, mnemonic) {
}

std::shared_ptr<laf::ButtonUI> Button::get_ui() const {
  return std::static_pointer_cast<laf::ButtonUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> Button::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

bool Button::is_default_button() const {
  if (auto &&root_pane = get_root_pane(shared_from_this())) {
    return root_pane->get_default_button() == shared_from_this();
  }
  return false;
}

}
