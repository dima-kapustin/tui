#include <tui++/ButtonModel.h>
#include <tui++/Screen.h>

namespace tui {

void ButtonModel::set_pressed(bool value) {
  if (value != is_pressed() and is_enabled()) {
    this->state.is_pressed = value;

    if (not is_pressed() and is_armed()) {
      auto modifiers = InputEvent::Modifiers { };
      auto current_event = Screen::get_event_queue().get_current_event();
      if (auto input_event = std::dynamic_pointer_cast<InputEvent>(current_event)) {
        modifiers = input_event->modifiers;
      } else if (auto action_event = std::dynamic_pointer_cast<ActionEvent>(current_event)) {
        modifiers = action_event->modifiers;
      }

      fire_event<ActionEvent>(shared_from_this(), get_action_command(), modifiers, Screen::get_event_queue().get_most_recent_event_time());
    }

    fire_event<ChangeEvent>(shared_from_this());
  }
}

void ButtonModel::set_armed(bool value) {
  if (is_armed() != value and (is_enabled() or (is_menu_item() and false/*and UIManager.getBoolean("MenuItem.disabledAreNavigable")*/))) {
    this->state.is_armed = value;
    fire_event<ChangeEvent>(shared_from_this());
  }
}

}
