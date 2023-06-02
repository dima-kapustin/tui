#pragma once

#include <mutex>

#include <tui++/EventQueue.h>

namespace tui {

class Window;

class Screen {
  static bool quit;
  static EventQueue event_queue;
  static std::shared_ptr<Event> last_focus_event;
  static std::vector<std::shared_ptr<Window>> windows;
  static std::mutex mutex;

private:
  static void add_window(const std::shared_ptr<Window> &window);
  static void remove_window(const std::shared_ptr<Window> &window);

  friend class Window;
public:
  static void run_event_loop();

  static void post(const std::shared_ptr<Event> &event);
  static void post(std::function<void()> fn);

  static std::shared_ptr<Window> get_top_window();
  static std::shared_ptr<Component> get_component_at(int x, int y);

  static std::shared_ptr<Event> get_last_focus_event() {
    return last_focus_event;
  }
};

}
