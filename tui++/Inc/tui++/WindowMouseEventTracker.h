#pragma once

#include <tui++/Event.h>

namespace tui {

class Window;

class WindowMouseEventTracker: public EventListener<Event> {
  Window *const window;

  bool isMouseInNativeContainer = false;

private:
  void track_mouse_enter_exit(const std::shared_ptr<Component> &target_over, Event &e);

public:
  WindowMouseEventTracker(Window *window) :
      window(window) {
  }

public:
  bool process_event(Event &e);

  void event_dispatched(Event &e) override;
};

}
