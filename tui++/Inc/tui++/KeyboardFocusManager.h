#pragma once

#include <map>
#include <memory>
#include <atomic>

namespace tui {

class Window;
class Component;
class FocusTraversalPolicy;

class KeyboardFocusManager {
  static std::atomic<std::shared_ptr<Component>> focus_owner;
  static std::atomic<std::shared_ptr<Window>> focused_window;
  static std::atomic<std::shared_ptr<Component>> current_focus_cycle_root;
  static std::atomic<std::shared_ptr<FocusTraversalPolicy>> default_focus_traversal_policy;

  static std::map<std::shared_ptr<Window>, std::weak_ptr<Component>> most_recent_focus_owners;

private:
  static void set_most_recent_focus_owner(const std::shared_ptr<Component> &component);

  static void set_most_recent_focus_owner(const std::shared_ptr<Window> &window, const std::shared_ptr<Component> &component);
  static std::shared_ptr<Component> get_most_recent_focus_owner(const std::shared_ptr<Window> &window);

  static bool request_focus(const std::shared_ptr<Component> &component, bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause);

  friend class Window;
  friend class Component;

public:
  static std::shared_ptr<Component> get_current_focus_cycle_root() {
    return current_focus_cycle_root.load();
  }

  static std::shared_ptr<FocusTraversalPolicy> get_default_focus_traversal_policy() {
    return default_focus_traversal_policy;
  }

  static void set_global_current_focus_cycle_root(const std::shared_ptr<Component> &new_focus_cycle_root) {
    auto old_focus_cycle_root = current_focus_cycle_root.exchange(new_focus_cycle_root);
//      firePropertyChange("currentFocusCycleRoot", oldFocusCycleRoot, newFocusCycleRoot);
  }

  static std::shared_ptr<Component> get_focus_owner() {
    return focus_owner.load();
  }

  static std::shared_ptr<Window> get_focused_window() {
    return focused_window.load();
  }

  static void clear_global_focus_owner();

  static bool dispatch_event(const std::shared_ptr<Event> &e);

  static void process_key_event(const std::shared_ptr<Component> &focused_component, KeyEvent &e);
};

}
