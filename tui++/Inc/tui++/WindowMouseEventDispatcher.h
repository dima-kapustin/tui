#pragma once

#include <memory>

#include <tui++/Event.h>

namespace tui {

class Window;
class Component;

class WindowMouseEventDispatcher: public std::enable_shared_from_this<WindowMouseEventDispatcher>, public EventListener<Event> {
  // The screen keeps a registered dispatcher alive through its listener list,
  // which can outlive the window it serves (e.g. a popup window that is
  // closed while the pointer is still over it). Hold the window weakly so the
  // dispatcher can tell when its window is gone instead of chasing a dangling
  // pointer.
  std::weak_ptr<Window> window;

  std::weak_ptr<Component> target_last_entered;
  std::weak_ptr<Component> mouse_event_target;
  bool mouse_over_window = false;

  EventTypeMask event_mask = EventTypeMask::NONE;

private:
  void start_listening_for_other_drags();
  void stop_listening_for_other_drags();

  void track_mouse_enter_exit(const std::shared_ptr<Component> &target_over, MouseEvent &e, bool inside_window);
  std::shared_ptr<Component> retarget_mouse_enter_exit(const std::shared_ptr<Component> &target_over, MouseEvent &e, const std::shared_ptr<Component> &last_entered, bool in_window);

  void retarget_mouse_event(const std::shared_ptr<Component> &target, MouseEvent &e);

  bool dispatch_event(MouseEvent &e);

public:
  explicit WindowMouseEventDispatcher(const std::shared_ptr<Window> &window) :
      window(window) {
  }

  ~WindowMouseEventDispatcher();

public:
  void enable_events(const EventTypeMask &event_mask) {
    this->event_mask |= event_mask;
  }

  // Drops the dispatcher from the screen's listener list and forgets the
  // pointer-over-window state (safe to call more than once). A window calls
  // this when it hides so a dispatcher never outlives the window it points at
  // and a later re-show starts tracking from a clean state.
  void unregister() {
    this->mouse_over_window = false;
    stop_listening_for_other_drags();
  }

  bool dispatch_event(Event &e);

  void event_dispatched(Event &e) override;
};

}
