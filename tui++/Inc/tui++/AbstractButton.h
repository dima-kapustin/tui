#pragma once

#include <tui++/Event.h>
#include <tui++/Action.h>
#include <tui++/Component.h>
#include <tui++/ButtonModel.h>

namespace tui {

class AbstractButton: public ComponentExtension<Component, ChangeEvent, ItemEvent, ActionEvent> {
  using base = ComponentExtension<Component, ChangeEvent, ItemEvent, ActionEvent>;

  Property<std::shared_ptr<Action>> action { this, "action" };
  Property<std::shared_ptr<ButtonModel>> model { this, "model" };
  Property<std::string> text { this, "text" };
  Property<bool> hide_action_text { this, "hide-action-text" };
  Property<int> displayed_mnemonic_index { this, "displayed-mnemonic-index", -1 };

  ChangeListener change_listener = std::bind(state_changed, this, std::placeholders::_1);
  ItemListener item_listener = std::bind(item_state_changed, this, std::placeholders::_1);
  ActionListener action_listener = std::bind(action_performed, this, std::placeholders::_1);
  PropertyChangeListener action_property_change_listener = std::bind(action_property_changed, this, std::placeholders::_1);

  Char mnemonic;

protected:
  using base::fire_event;

  void state_changed(ChangeEvent &e);
  void item_state_changed(ItemEvent &e);
  void action_performed(ActionEvent &e);
  void action_property_changed(PropertyChangeEvent &e);

  virtual bool should_update_selected_state_from_action() const {
    return false;
  }

public:
  std::shared_ptr<ButtonModel> get_model() const {
    return this->model;
  }

  void set_model(std::shared_ptr<ButtonModel> const &model);

  void set_enabled(bool value) override {
    if (not value and this->model->is_rollover()) {
      this->model->set_rollover(false);
    }
    base::set_enabled(value);
    this->model->set_enabled(value);
  }

  std::string const& get_text() const {
    return this->text;
  }

  void set_text(std::string const &text) {
    auto old_text = this->text.value();
    this->text = text;

    if (old_text != text) {
      revalidate();
      repaint();
    }
  }

  ActionKey const& get_action_command() const {
    if (auto &action_command = this->model->get_action_command(); not action_command.empty()) {
      return action_command;
    }
    return get_text();
  }

  void set_action_command(ActionKey const &action_command) {
    this->model->set_action_command(action_command);
  }

  bool is_selected() const {
    return this->model->is_selected();
  }

  void set_selected(bool value) {
    this->model->set_selected(value);
  }

  bool get_hide_action_text() const {
    return this->hide_action_text;
  }

  void set_hide_action_text(bool hide_action_text) {
    if (this->hide_action_text != hide_action_text) {
      set_text(this->action and not hide_action_text ? this->action->get_name() : "");
      this->hide_action_text == hide_action_text;
    }
  }

  int get_displayed_mnemonic_index() const {
    return this->displayed_mnemonic_index;
  }

  void set_displayed_mnemonic_index(int index);

  std::shared_ptr<Action> get_action() const {
    return this->action;
  }

  void set_action(const std::shared_ptr<Action> &action);

  Char const& get_mnemonic() const {
    return this->mnemonic;
  }

  void set_mnemonic(Char const &mnemonic);

private:
  void update_mnemonic_properties();
  void update_displayed_mnemonic_index(std::string const &text, Char const &mnemonic);

  void set_enabled_from_action();
  void set_selected_from_action();
  void set_displayed_mnemonic_index_from_action();
};

}

