#include <tui++/AbstractButton.h>

namespace tui {

void AbstractButton::set_model(std::shared_ptr<ButtonModel> const &model) {
  if (this->model) {
    this->model->remove_event_listener(this->change_event_listener);
    this->model->remove_event_listener(this->action_event_listener);
    this->model->remove_event_listener(this->item_event_listener);
  }

  if (model) {
    model->add_event_listener(this->change_event_listener);
    model->add_event_listener(this->action_event_listener);
    model->add_event_listener(this->item_event_listener);
  }

  this->model = model;
}

void AbstractButton::state_changed(ChangeEvent &e) {
  fire_event<ChangeEvent>(shared_from_this());
}

void AbstractButton::item_state_changed(ItemEvent &e) {
  fire_event<ItemEvent>(shared_from_this(), e.type());
}

void AbstractButton::action_performed(ActionEvent &e) {
  auto &action_command = e.action_command.empty() ? get_action_command() : e.action_command;
  fire_event<ActionEvent>(shared_from_this(), action_command, e.modifiers, e.when);
}

}
