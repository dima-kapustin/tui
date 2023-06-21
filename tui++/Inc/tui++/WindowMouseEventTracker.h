#pragma once

#include <memory>

#include <tui++/Event.h>

namespace tui {

class Window;

class WindowMouseEventTracker: public std::enable_shared_from_this<WindowMouseEventTracker>, public EventListener<Event> {
  Window *const window;

  bool isMouseInNativeContainer = false;

private:
  void start_listening_for_other_drags();
  void stop_listening_for_other_drags();

public:
  WindowMouseEventTracker(Window *window) :
      window(window) {
  }

public:
  bool process_event(Event &e);

  void event_dispatched(Event &e) override;
};

}
