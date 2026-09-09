#pragma once

#include <tui++/AbstractButton.h>
#include <tui++/ToggleButtonModel.h>

namespace tui {
namespace laf {
class ToggleButtonUI;
}

class ToggleButton: public AbstractButton {
  using base = AbstractButton;

protected:
  // Swing's JToggleButton: the default model is a ToggleButtonModel, which
  // toggles the selected state when the button is clicked (a plain Button's
  // model only stays selected while it is pressed).
  ToggleButton(std::string const &text = "") :
      ToggleButton(text, Char { }) {
  }

  ToggleButton(std::string const &text, Char const &mnemonic) :
      base(std::make_shared<ToggleButtonModel>(), text, mnemonic) {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

public:
  void request_focus(FocusEvent::Cause cause) override {
    get_group_selection(cause)->request_focus_unconditionally(cause);
  }

  bool request_focus_in_window(FocusEvent::Cause cause) override {
    return get_group_selection(cause)->request_focus_in_window_unconditionally(cause);
  }

  std::shared_ptr<laf::ToggleButtonUI> get_ui() const;

protected:
  std::shared_ptr<laf::ComponentUI> create_ui() override;

  bool should_update_selected_state_from_action() const override {
    return true;
  }

private:
  std::shared_ptr<ToggleButton> get_group_selection(FocusEvent::Cause cause);

  void request_focus_unconditionally(FocusEvent::Cause cause) {
    base::request_focus(cause);
  }

  bool request_focus_in_window_unconditionally(FocusEvent::Cause cause) {
    return base::request_focus_in_window(cause);
  }
};

}
