#pragma once

#include <tui++/EventQueue.h>

namespace tui {

class Window;

class Screen {
  static bool quit;
  static EventQueue event_queue;
  static std::shared_ptr<Event> last_focus_event;

public:
  static void run_event_loop();

  static void post(const std::shared_ptr<Event> &event);
  static void post(std::function<void()> fn);

  static std::shared_ptr<Window> get_top_window();

  static std::shared_ptr<Event> get_last_focus_event() {
    return last_focus_event;
  }
};

}
