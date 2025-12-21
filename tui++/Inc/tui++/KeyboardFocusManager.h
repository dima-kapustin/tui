#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <atomic>

#include <tui++/Object.h>

#include <atomic>
#include <memory>

namespace tui {

class Window;
class Component;
class FocusTraversalPolicy;

class KeyboardFocusManager {
  std::mutex mutex;
  std::atomic<std::shared_ptr<Component>> focus_owner;
  std::atomic<std::shared_ptr<Component>> permanent_focus_owner;
  std::atomic<std::shared_ptr<Window>> focused_window;
  std::atomic<std::shared_ptr<Window>> active_window;
  std::atomic<std::shared_ptr<Component>> current_focus_cycle_root;
  std::atomic<std::shared_ptr<FocusTraversalPolicy>> default_focus_traversal_policy;
  std::atomic<std::weak_ptr<Component>> real_opposite_component;
  std::atomic<std::weak_ptr<Window>> real_opposite_window;
  std::atomic<std::shared_ptr<Component>> restore_focus_to;
  std::atomic<unsigned> in_send_event = false;

  std::map<std::shared_ptr<const Window>, std::weak_ptr<Component>> most_recent_focus_owners;
  std::vector<std::shared_ptr<std::unordered_set<KeyStroke>>> default_focus_traversal_keys;

  bool consume_next_key_typed = false;

public:
  enum FocusTraversalKeys {
    FORWARD_TRAVERSAL_KEYS,
    BACKWARD_TRAVERSAL_KEYS,
    UP_CYCLE_TRAVERSAL_KEYS,
    DOWN_CYCLE_TRAVERSAL_KEYS
  };

  static inline std::shared_ptr<KeyboardFocusManager> single = std::make_shared<KeyboardFocusManager>();

public:
  KeyboardFocusManager();

  void add_property_change_listener(const char *property_name, PropertyChangeListener const &listener);
  void remove_property_change_listener(const char *property_name, PropertyChangeListener const &listener);

  void add_property_change_listener(PropertyChangeListener const &listener);
  void remove_property_change_listener(PropertyChangeListener const &listener);

  std::shared_ptr<FocusTraversalPolicy> get_default_focus_traversal_policy() {
    return this->default_focus_traversal_policy;
  }

  std::shared_ptr<Component> get_current_focus_cycle_root() {
    return this->current_focus_cycle_root.load();
  }

  void set_current_focus_cycle_root(const std::shared_ptr<Component> &component) {
    auto old_focus_cycle_root = this->current_focus_cycle_root.exchange(component);
//      firePropertyChange("current_focus_cycle_root", old_focus_cycle_root, component);
  }

  std::shared_ptr<Component> get_focus_owner() {
    return this->focus_owner;
  }

  std::shared_ptr<Window> get_focused_window() {
    return this->focused_window;
  }

  std::shared_ptr<Window> get_active_window() {
    return this->active_window;
  }

  bool dispatch_event(Event &e);

  void process_key_event(const std::shared_ptr<Component> &focused_component, KeyEvent &e);

  std::shared_ptr<const std::unordered_set<KeyStroke>> get_default_focus_traversal_keys(FocusTraversalKeys id);

  void redispatch_event(const std::shared_ptr<Component> &target, Event &e);

  void clear_most_recent_focus_owner(const std::shared_ptr<Component> &c);

  bool is_auto_focus_transfer_enabled();

  std::shared_ptr<Component> get_permanent_focus_owner();
  void set_permanent_focus_owner(std::shared_ptr<Component> const &c);

private:
  void set_most_recent_focus_owner(const std::shared_ptr<Component> &component);

  void set_most_recent_focus_owner(const std::shared_ptr<Window> &window, const std::shared_ptr<Component> &component);
  std::shared_ptr<Component> get_most_recent_focus_owner(const std::shared_ptr<const Window> &window);

  void set_focus_owner(const std::shared_ptr<Component> &component) {
    this->focus_owner = component;
  }

  void set_focused_window(const std::shared_ptr<Window> &window) {
    return this->focused_window = window;
  }

  void set_active_window(const std::shared_ptr<Window> &window) {
    this->active_window = window;
  }

  void clear_focus_owner();

  bool request_focus(const std::shared_ptr<Component> &component, bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause);

  void consume_traversal_key(KeyEvent &e) {
    e.consumed = true;
    this->consume_next_key_typed = (e.id == KeyEvent::KEY_PRESSED) and not e.is_action_key();
  }

  bool consume_processed_key_event(KeyEvent &e) {
    if ((e.id == KeyEvent::KEY_TYPED) and this->consume_next_key_typed) {
      e.consumed = true;
      this->consume_next_key_typed = false;
      return true;
    }
    return false;
  }

  void focus_previous_component(const std::shared_ptr<Component> &component);
  void focus_next_component(const std::shared_ptr<Component> &component);
  void up_focus_cycle(const std::shared_ptr<Component> &component);
  void down_focus_cycle(const std::shared_ptr<Component> &component);

  /*
   * This series of restoreFocus methods is used for recovering from a
   * rejected focus or activation change. Rejections typically occur when
   * the user attempts to focus a non-focusable Component or Window.
   */
  void restore_focus(FocusEvent &e, const std::shared_ptr<Window> &new_focused_window);
  void restore_focus(WindowEvent &e);
  bool restore_focus(const std::shared_ptr<Window> &window, const std::shared_ptr<Component> &vetoed_component, bool clear_on_failure);
  bool restore_focus(const std::shared_ptr<Component> &to_focus, bool clear_on_failure) {
    return do_restore_focus(to_focus, nullptr, clear_on_failure);
  }
  bool do_restore_focus(const std::shared_ptr<Component> &to_focus, const std::shared_ptr<Component> &vetoed_component, bool clear_on_failure);

  template<typename T, typename Component, typename ... Args>
  void send_event(const std::shared_ptr<Component> &to, Args &&... args) {
    auto e = T { to, std::forward<Args>(args)... };
    this->in_send_event += 1;
    to->dispatch_event(e);
    this->in_send_event -= 1;
  }

  template<typename T, typename Component, typename ... Args>
  void redispatch_event(const std::shared_ptr<Component> &to, Args &&... args) {
    auto e = T { to, std::forward<Args>(args)... };
    redispatch_event(to, e);
  }

  friend class Window;
  friend class Component;
};

}
