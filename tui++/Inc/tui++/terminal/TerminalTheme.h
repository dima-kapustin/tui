#pragma once

#include <tui++/Theme.h>

namespace tui {

class TerminalTheme: public Theme {
protected:
  virtual void init() override;

private:
  void init_system_color_defaults();
  void init_component_defaults();
};

}
