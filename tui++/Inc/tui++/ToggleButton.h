#pragma once

#include <tui++/AbstractButton.h>

namespace tui {
namespace laf {
class ToggleButtonUI;
}
class ToggleButton: public AbstractButton {
  using base = AbstractButton;
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
