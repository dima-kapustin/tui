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

  std::shared_ptr<laf::ToggleButtonUI> get_ui() const;

protected:
  std::shared_ptr<laf::ComponentUI> create_ui() override;

private:
  std::shared_ptr<ToggleButton> get_group_selection(FocusEvent::Cause cause);

  void request_focus_unconditionally(FocusEvent::Cause cause) {
    base::request_focus(cause);
  }
};

}
