#pragma once

#include <tui++/Screen.h>

namespace tui::terminal {

class Terminal;

class TerminalScreen: public Screen {
  Terminal &terminal;

private:
  TerminalScreen(Terminal &terminal) noexcept :
      terminal(terminal) {
  }

  friend class Terminal;

public:
  virtual void run_event_loop() override;

  virtual Dimension get_size() const override;
};

}

