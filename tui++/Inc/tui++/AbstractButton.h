#pragma once

#include <tui++/Event.h>
#include <tui++/Action.h>
#include <tui++/Component.h>
#include <tui++/ButtonModel.h>

namespace tui {

class AbstractButton: public ComponentExtension<Component, ActionEvent, ItemEvent> {
  Property<std::shared_ptr<Action>> action { this, "action" };
  Property<std::shared_ptr<ButtonModel>> model { this, "model" };
  Property<std::string> text { this, "text" };

  ChangeListener state_listener = std::bind(state_changed, this, std::placeholders::_1);
  ItemListener item_listener = std::bind(item_state_changed, this, std::placeholders::_1);
  ActionListener action_listener = std::bind(action_performed, this, std::placeholders::_1);

protected:
//  void process_event(FocusEvent &e) override {
//
//  }
//
//  void process_event(ItemEvent &e) override {
//
//  }

  void state_changed(ChangeEvent &e) {

  }

  void item_state_changed(ItemEvent &e) {

  }

  void action_performed(ActionEvent &e) {

  }

public:
  std::shared_ptr<ButtonModel> get_model() const {
    return this->model;
  }

  void set_model(std::shared_ptr<ButtonModel> const &model) {

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

  bool is_selected() const {
    return this->model->is_selected();
  }

  void set_selected(bool value) {
    this->model->set_selected(value);
  }

  void set_action(const std::shared_ptr<Action> &action) {
//    this->action = action;
    static_assert(detail::has_bool_operator_v<std::shared_ptr<Action>>);
    if (this->action != action) {
      if (this->action) {
        remove_event_listener(this->action.value());
//            oldValue->removePropertyChangeListener(actionPropertyChangeListener);
//            actionPropertyChangeListener = null;
      }
//        configurePropertiesFromAction(action);
      this->action = action;
      if (action) {
        add_event_listener(this->action.value());
        // Reverse linkage:
//            actionPropertyChangeListener = createActionPropertyChangeListener(action);
//            action.addPropertyChangeListener(actionPropertyChangeListener);
      }
    }
  }
};

}

