#pragma once

#include <tui++/Component.h>

namespace tui {

class Window: public Component {
public:
  bool is_displayable() const override {
    return true;
  }
};

}
