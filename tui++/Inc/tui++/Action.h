#pragma once

#include <tui++/Event.h>

namespace tui {

class Action {
public:
  virtual ~Action() {
  }

  virtual bool is_enabled() const = 0;

  virtual void action_performed(const ActionEvent &event) = 0;
};

class BasicAction: public Action {

};

}
