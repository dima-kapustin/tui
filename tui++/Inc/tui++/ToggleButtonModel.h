#pragma once

#include <tui++/ButtonModel.h>

namespace tui {

class ToggleButtonModel: public ButtonModel {
  using base = ButtonModel;
public:
  void set_selected(bool value) override;
  void set_pressed(bool value) override;
};

}
