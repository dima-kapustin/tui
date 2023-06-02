#include <chrono>
#include <iostream>

#include <tui++/Screen.h>
#include <tui++/Terminal.h>

namespace tui {

bool Screen::quit = false;
EventQueue Screen::event_queue;
std::shared_ptr<Event> Screen::last_focus_event;

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

void Screen::run_event_loop() {
  while (not quit) {
    Terminal::read_events();
    if (auto event = event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      std::cout << *event << std::endl;
    }
  }
}

void Screen::post(const std::shared_ptr<Event> &event) {
  event_queue.push(event);
  if (event->type == Event::FOCUS) {
    last_focus_event = event;
  }
}

void Screen::post(std::function<void()> fn) {
  post(std::make_shared<Event>(fn));
}

}
