#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalScreen.h>
#include <tui++/terminal/TerminalGraphics.h>

#include <iostream>

namespace tui::terminal {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

void TerminalScreen::run_event_loop() {
  while (not this->quit) {
    this->terminal.read_events();
    if (auto event = this->event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      std::cout << *event << std::endl;
    }
  }
}

void TerminalScreen::refresh() {
  auto size = this->size;
  this->size = this->terminal.get_size();
  if (size != this->size) {
    this->chars.resize(this->size.height);
    for (auto &&row : this->chars) {
      row.resize(this->size.width);
    }
  }

  auto g = TerminalGraphics { *this };
  paint(g);
}

}
