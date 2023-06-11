#pragma once

#include <tui++/Component.h>

namespace tui {

class Panel: public Component {
  using base = Component;

public:
  Panel();

  Panel(const std::shared_ptr<Layout> &layout) {
    set_layout(layout);
  }

public:
  void paint(Graphics &g);
};

}

