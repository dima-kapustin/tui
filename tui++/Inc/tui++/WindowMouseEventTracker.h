#pragma once

#include <memory>

#include <tui++/Event.h>

namespace tui {

class Window;
class Component;

class WindowMouseEventTracker: public std::enable_shared_from_this<WindowMouseEventTracker>, public EventListener<Event> {
  Window *const window;

  std::weak_ptr<Component> target_last_entered;
  bool isMouseInNativeContainer = false;

private:
  void start_listening_for_other_drags();
  void stop_listening_for_other_drags();

  std::shared_ptr<Component> retarget_mouse_enter_exit(const std::shared_ptr<Component> &target_over, Event &e, std::shared_ptr<Component> lastEntered, bool in_window);

public:
  WindowMouseEventTracker(Window *window) :
      window(window) {
  }

public:
  bool process_event(Event &e);

  void event_dispatched(Event &e) override;
};

}
