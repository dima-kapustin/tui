#include <tui++/TerminalScreen.h>

#include <tui++/Terminal.h>

#include <iostream>

namespace tui {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

void TerminalScreen::run_event_loop() {
  while (not this->quit) {
    this->terminal.read_events();
    if (auto event = this->event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      std::cout << *event << std::endl;
    }
  }
}

Dimension TerminalScreen::get_size() const {
  return this->terminal.get_size();
}

}
