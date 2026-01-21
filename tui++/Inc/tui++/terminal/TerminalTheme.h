#pragma once

#include <tui++/Theme.h>

namespace tui {

class TerminalTheme: public Theme {
protected:
  virtual void install() override;

private:
  void install_system_colors();
};

}
