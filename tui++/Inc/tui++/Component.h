#pragma once

#include <tui++/Graphics.h>

namespace tui {

class Component {
public:
  virtual ~Component() {

  }

  virtual void paint(Graphics &g) = 0;
};

}
