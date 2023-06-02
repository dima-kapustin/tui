#pragma once

#include <tui++/Component.h>

namespace tui {

class Window: public Component {
  using base = Component;

protected:
  void paint_children(Graphics &g) override;

public:
  bool is_displayable() const override {
    return true;
  }

  void paint(Graphics &g) override {
    validate();
    base::paint(g);
    request_focus();
  }
};

}
