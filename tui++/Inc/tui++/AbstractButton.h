#pragma once

#include <tui++/Event.h>
#include <tui++/Action.h>
#include <tui++/Component.h>

namespace tui {

class AbstractButton: public ComponentExtension<Component, ActionEvent, ItemEvent> {
  Property<std::shared_ptr<Action>> action { this, "action" };

protected:
  void process_event(FocusEvent &e) override {

  }

  void process_event(ItemEvent &e) override {

  }

public:
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
      if (action) {
        // Don't add if it is already a listener
//            if (!isListener(ActionListener.class, action)) {
//                addActionListener(action);
//            }
        // Reverse linkage:
//            actionPropertyChangeListener = createActionPropertyChangeListener(action);
//            action.addPropertyChangeListener(actionPropertyChangeListener);
      }
      this->action = action;
    }
  }
};

}

