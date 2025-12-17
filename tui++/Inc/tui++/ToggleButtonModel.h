#pragma once

#include <ButtonModel.h>

namespace tui {

class ToggleButtonModel: public ButtonModel {
  using base = ButtonModel;
public:
  void set_selected(bool value) override {
    if (auto group = get_group()) {
      // use the group model instead
      group->set_selected(shared_from_this(), value);
      value = group.is_selected(this);
    }
    base::set_selected(value);
  }
};

}
