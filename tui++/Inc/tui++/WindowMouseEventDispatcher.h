#pragma once

#include <memory>

#include <tui++/Event.h>

namespace tui {

class Window;
class Component;

class WindowMouseEventDispatcher: public std::enable_shared_from_this<WindowMouseEventDispatcher>, public EventListener<Event> {
  Window *const window;

  std::weak_ptr<Component> target_last_entered;
  bool mouse_over_window = false;

  EventTypeMask event_mask = EventTypeMask::NONE;

private:
  void start_listening_for_other_drags();
  void stop_listening_for_other_drags();

  std::shared_ptr<Component> retarget_mouse_enter_exit(const std::shared_ptr<Component> &target_over, MouseEventBase &e, const std::shared_ptr<Component> &last_entered, bool in_window);

  void retarget_mouse_event(const std::shared_ptr<Component> &target, MouseEventBase &e);

public:
  WindowMouseEventDispatcher(Window *window) :
      window(window) {
  }

public:
  void enable_events(const EventTypeMask &event_mask) {
    this->event_mask |= event_mask;
  }

  bool dispatch_event(MouseEventBase &e);

  void event_dispatched(Event &e) override;
};

}
