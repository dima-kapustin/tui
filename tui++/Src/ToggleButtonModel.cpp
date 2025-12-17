#include <tui++/ToggleButtonModel.h>
#include <tui++/ButtonGroup.h>

#include <tui++/Screen.h>

namespace tui {

void ToggleButtonModel::set_selected(bool value) {
  if (auto group = get_group()) {
    // use the group model instead
    group->set_selected(shared_from_this(), value);
    value = group->is_selected(shared_from_this());
  }

  if (value != is_selected()) {
    state.is_selected = value;

    fire_event<ChangeEvent>(shared_from_this());
    fire_event<ItemEvent>(shared_from_this(), value ? ItemEvent::SELECTED : ItemEvent::DESELECTED);
  }
}

void ToggleButtonModel::set_pressed(bool value) {
  if (is_pressed() != value and is_enabled()) {
    if (not value and is_armed()) {
      set_selected(not is_selected());
    }

    state.is_pressed = value;

    fire_event<ChangeEvent>(shared_from_this());

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
  }
}

}
