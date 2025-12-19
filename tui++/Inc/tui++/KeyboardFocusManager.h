#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <atomic>

namespace tui {

class Window;
class Component;
class FocusTraversalPolicy;

class KeyboardFocusManager {
  static std::mutex mutex;
  static std::atomic<std::shared_ptr<Component>> focus_owner;
  static std::atomic<std::shared_ptr<Component>> permanent_focus_owner;
  static std::atomic<std::shared_ptr<Window>> focused_window;
  static std::atomic<std::shared_ptr<Window>> active_window;
  static std::atomic<std::shared_ptr<Component>> current_focus_cycle_root;
  static std::atomic<std::shared_ptr<FocusTraversalPolicy>> default_focus_traversal_policy;
  static std::atomic<std::weak_ptr<Component>> real_opposite_component;
  static std::atomic<std::weak_ptr<Window>> real_opposite_window;
  static std::atomic<std::shared_ptr<Component>> restoreFocusTo;
  static std::atomic<unsigned> in_send_event;

  static std::map<std::shared_ptr<const Window>, std::weak_ptr<Component>> most_recent_focus_owners;
  static std::vector<std::shared_ptr<std::unordered_set<KeyStroke>>> default_focus_traversal_keys;

  static bool consume_next_key_typed;

private:
  static void set_most_recent_focus_owner(const std::shared_ptr<Component> &component);

  static void set_most_recent_focus_owner(const std::shared_ptr<Window> &window, const std::shared_ptr<Component> &component);
  static std::shared_ptr<Component> get_most_recent_focus_owner(const std::shared_ptr<const Window> &window);

  static void set_focus_owner(const std::shared_ptr<Component> &component) {
    focus_owner = component;
  }

  static void set_permanent_focus_owner(const std::shared_ptr<Component> &component) {
    permanent_focus_owner = component;
    set_most_recent_focus_owner(component);
  }

  static void set_focused_window(const std::shared_ptr<Window> &window) {
    return focused_window = window;
  }

  static void set_active_window(const std::shared_ptr<Window> &window) {
    active_window = window;
  }

  static void clear_focus_owner();

  static bool request_focus(const std::shared_ptr<Component> &component, bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause);

  static void consume_traversal_key(KeyEvent &e) {
    e.consumed = true;
    consume_next_key_typed = (e.id == KeyEvent::KEY_PRESSED) and not e.is_action_key();
  }

  static bool consume_processed_key_event(KeyEvent &e) {
    if ((e.id == KeyEvent::KEY_TYPED) and consume_next_key_typed) {
      e.consumed = true;
      consume_next_key_typed = false;
      return true;
    }
    return false;
  }

  static void focus_previous_component(const std::shared_ptr<Component> &component);
  static void focus_next_component(const std::shared_ptr<Component> &component);
  static void up_focus_cycle(const std::shared_ptr<Component> &component);
  static void down_focus_cycle(const std::shared_ptr<Component> &component);

  /*
   * This series of restoreFocus methods is used for recovering from a
   * rejected focus or activation change. Rejections typically occur when
   * the user attempts to focus a non-focusable Component or Window.
   */
  static void restore_focus(FocusEvent &e, const std::shared_ptr<Window> &new_focused_window);
  static void restore_focus(WindowEvent &e);
  static bool restore_focus(const std::shared_ptr<Window> &window, const std::shared_ptr<Component> &vetoed_component, bool clear_on_failure);
  static bool restore_focus(const std::shared_ptr<Component> &to_focus, bool clear_on_failure) {
    return do_restore_focus(to_focus, nullptr, clear_on_failure);
  }
  static bool do_restore_focus(const std::shared_ptr<Component> &to_focus, const std::shared_ptr<Component> &vetoed_component, bool clear_on_failure);

  template<typename T, typename Component, typename ... Args>
  static void send_event(const std::shared_ptr<Component> &to, Args &&... args) {
    auto e = T { to, std::forward<Args>(args)... };
    in_send_event += 1;
    to->dispatch_event(e);
    in_send_event -= 1;
  }

  template<typename T, typename Component, typename ... Args>
  static void redispatch_event(const std::shared_ptr<Component> &to, Args &&... args) {
    auto e = T { to, std::forward<Args>(args)... };
    redispatch_event(to, e);
  }

  friend class Window;
  friend class Component;

public:
  enum FocusTraversalKeys {
    FORWARD_TRAVERSAL_KEYS,
    BACKWARD_TRAVERSAL_KEYS,
    UP_CYCLE_TRAVERSAL_KEYS,
    DOWN_CYCLE_TRAVERSAL_KEYS
  };

public:
  static std::shared_ptr<FocusTraversalPolicy> get_default_focus_traversal_policy() {
    return default_focus_traversal_policy;
  }

  static std::shared_ptr<Component> get_current_focus_cycle_root() {
    return current_focus_cycle_root.load();
  }

  static void set_current_focus_cycle_root(const std::shared_ptr<Component> &component) {
    auto old_focus_cycle_root = current_focus_cycle_root.exchange(component);
//      firePropertyChange("current_focus_cycle_root", old_focus_cycle_root, component);
  }

  static std::shared_ptr<Component> get_focus_owner() {
    return focus_owner;
  }

  static std::shared_ptr<Window> get_focused_window() {
    return focused_window;
  }

  static std::shared_ptr<Window> get_active_window() {
    return active_window;
  }

  static bool dispatch_event(Event &e);

  static void process_key_event(const std::shared_ptr<Component> &focused_component, KeyEvent &e);

  static std::shared_ptr<const std::unordered_set<KeyStroke>> get_default_focus_traversal_keys(FocusTraversalKeys id);

  static void redispatch_event(const std::shared_ptr<Component> &target, Event &e);

  static void clear_most_recent_focus_owner(const std::shared_ptr<Component> &c);
};

}
