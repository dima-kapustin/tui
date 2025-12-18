#include <tui++/ToggleButton.h>

#include <tui++/ButtonGroup.h>

#include <tui++/lookandfeel/ToggleButtonUI.h>

namespace tui {
std::shared_ptr<laf::ToggleButtonUI> ToggleButton::get_ui() const {
  return std::static_pointer_cast<laf::ToggleButtonUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> ToggleButton::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

std::shared_ptr<ToggleButton> ToggleButton::get_group_selection(FocusEvent::Cause cause) {
  switch (cause) {
  case FocusEvent::Cause::ACTIVATION:
  case FocusEvent::Cause::TRAVERSAL:
  case FocusEvent::Cause::TRAVERSAL_UP:
  case FocusEvent::Cause::TRAVERSAL_DOWN:
  case FocusEvent::Cause::TRAVERSAL_FORWARD:
  case FocusEvent::Cause::TRAVERSAL_BACKWARD:
    if (auto model = get_model()) {
      if (auto group = model->get_group(); group and group->get_selection() and not group->is_selected(model)) {
        for (auto &&member : group->get_buttons()) {
          if (group->is_selected(member->get_model())) {
            if (auto toggle_button = std::dynamic_pointer_cast<ToggleButton>(member); toggle_button and //
                toggle_button->is_visible() and toggle_button->is_displayable() and //
                toggle_button->is_enabled() and toggle_button->is_focusable()) {
              return toggle_button;
            }
            break;
          }
        }
      }
    }
    return {};
  default:
    return std::static_pointer_cast<ToggleButton>(shared_from_this());
  }
}

}
