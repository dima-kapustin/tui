#include <tui++/AbstractButton.h>

#include <tui++/ButtonGroup.h>

namespace tui {

void AbstractButton::set_model(std::shared_ptr<ButtonModel> const &model) {
  if (this->model) {
    this->model->remove_listener(this->change_listener);
    this->model->remove_listener(this->action_listener);
    this->model->remove_listener(this->item_listener);
  }

  if (model) {
    model->add_listener(this->change_listener);
    model->add_listener(this->action_listener);
    model->add_listener(this->item_listener);
  }

  this->model = model;
}

void AbstractButton::state_changed(ChangeEvent &e) {
  if (is_enabled() != this->model->is_enabled()) {
    set_enabled(model->is_enabled());
  }
  update_mnemonic_properties();
  fire_event<ChangeEvent>(shared_from_this());
  repaint();
}

void AbstractButton::item_state_changed(ItemEvent &e) {
  fire_event<ItemEvent>(shared_from_this(), e.type());
  if (should_update_selected_state_from_action()) {
    if (this->action) {
      this->action->set_selected(is_selected());
    }
  }
}

void AbstractButton::action_performed(ActionEvent &e) {
  auto &action_command = e.action_command.empty() ? get_action_command() : e.action_command;
  fire_event<ActionEvent>(shared_from_this(), action_command, e.modifiers, e.when);
}

void AbstractButton::action_property_changed(PropertyChangeEvent &e) {
  if (e.property_name == Action::NAME) {
    if (not get_hide_action_text()) {
      set_text(this->action->get_name());
    }
  } else if (e.property_name == "enabled") {
    set_enabled_from_action();
  } else if (e.property_name == Action::SHORT_DESCRIPTION) {
    set_tool_tip_text(this->action->get_short_description());
  } else if (e.property_name == Action::MNEMONIC) {
    set_mnemonic(this->action->get_mnemonic());
  } else if (e.property_name == Action::ACTION_COMMAND) {
    set_text(this->action->get_action_command());
  } else if (e.property_name == Action::SELECTED and should_update_selected_state_from_action()) {
    set_selected_from_action();
  } else if (e.property_name == Action::DISPLAYED_MNEMONIC_INDEX) {
    set_displayed_mnemonic_index_from_action();
  }
}

void AbstractButton::set_action(const std::shared_ptr<Action> &action) {
  static_assert(detail::has_bool_operator_v<std::shared_ptr<Action>>);
  if (this->action != action) {
    auto old_action = this->action.value();
    this->action = action;
    if (old_action) {
      remove_listener(old_action);
      old_action->remove_property_change_listener(this->action_property_change_listener);
    }

    set_mnemonic(this->action->get_mnemonic());
    set_text(not get_hide_action_text() ? this->action->get_name() : std::string { });
    set_tool_tip_text(this->action->get_short_description());
    set_action_command(this->action->get_action_command());
    set_enabled_from_action();
    set_displayed_mnemonic_index_from_action();
    if (should_update_selected_state_from_action()) {
      set_selected_from_action();
    }

    if (action) {
      add_listener(this->action.value());
      this->action->add_property_change_listener(this->action_property_change_listener);
    }
  }
}

void AbstractButton::set_mnemonic(Char const &mnemonic) {
  this->model->set_mnemonic(mnemonic);
  update_mnemonic_properties();
}

void AbstractButton::set_displayed_mnemonic_index(int index) {
  auto const &text = get_text();
  if (index < -1 or index >= (int) text.length()) {
    throw std::invalid_argument(std::format("index == {}", index));
  }

  auto old_index = this->displayed_mnemonic_index;
  this->displayed_mnemonic_index = index;
  if (old_index != index) {
    revalidate();
    repaint();
  }
}

void AbstractButton::update_mnemonic_properties() {
  auto old_mnemonic = this->mnemonic;
  this->mnemonic = this->model->get_mnemonic();
  if (this->mnemonic != old_mnemonic) {
    update_displayed_mnemonic_index(get_text(), this->mnemonic);

    revalidate();
    repaint();
  }
}

void AbstractButton::update_displayed_mnemonic_index(std::string const &text, Char const &mnemonic) {
  auto index = -1;
  if (auto pos = text.find(mnemonic); pos != text.npos) {
    index = (int) pos;
  }
  set_displayed_mnemonic_index(index);
}

void AbstractButton::set_enabled_from_action() {
  base::set_enabled(this->action->is_enabled());
}

void AbstractButton::set_selected_from_action() {
  if (auto selected = this->action->is_selected(); selected != is_selected()) {
    set_selected(selected);
    if (not selected and is_selected() and this->model) {
      if (auto group = this->model->get_group()) {
        group->clear_selection();
      }
    }
  }
}

void AbstractButton::set_displayed_mnemonic_index_from_action() {
  auto index = this->action->get_displayed_mnemonic_index();
  if (auto const &text = get_text(); (int) text.length() <= index) {
    index = -1;
  }
  set_displayed_mnemonic_index(index);
}

}
