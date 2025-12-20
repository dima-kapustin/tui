#pragma once

#include <tui++/Component.h>
#include <tui++/WindowMouseEventDispatcher.h>

#include <tui++/event/WindowEvent.h>

namespace tui {

enum class WindowType {
  NORMAL,
  POPUP,
  UTILITY
};

class Screen;
class KeyboardFocusManager;

class Window: public ComponentExtension<Component, WindowEvent> {
  using base = Component;

  Screen &screen;
  std::shared_ptr<Window> owner;
  const WindowType type;

  std::vector<std::weak_ptr<Window>> owned_windows;
  std::shared_ptr<WindowMouseEventDispatcher> mouse_event_dispatcher;

  Property<bool> focusable_window_state { this, "focusable-window-state", true };
  Property<bool> always_on_top { this, "always-on-top", false };

  std::weak_ptr<Component> temporary_lost_component;

  struct {
    unsigned in_show :1 = false;
    unsigned before_first_show :1 = true;
    unsigned opened :1 = false;
    unsigned packed :1 = false;
  };

private:
  std::shared_ptr<Component> get_temporary_lost_component() const {
    return this->temporary_lost_component.lock();
  }

  std::shared_ptr<Component> set_temporary_lost_component(const std::shared_ptr<Component> &component) {
    auto prev = get_temporary_lost_component();
    // Check that "component" is an acceptable focus owner and don't store it otherwise
    // - or later we will have problems with opposite while handling  WINDOW_GAINED_FOCUS
    if (not component or component->can_be_focus_owner()) {
      this->temporary_lost_component = component;
    } else {
      this->temporary_lost_component.reset();
    }
    return prev;
  }

  void to_front();

  void enable_events_for_dispatching(EventTypeMask event_mask) {
    if (this->mouse_event_dispatcher) {
      this->mouse_event_dispatcher->enable_events(event_mask);
    }
  }

  void add_owned_window(const std::shared_ptr<Window> &w);
  void remove_owned_window(const std::shared_ptr<Window> &w);

  friend class Component;
  friend class KeyboardFocusManager;
  friend class WindowMouseEventDispatcher;

protected:
  Window(Screen &screen, WindowType type = WindowType::NORMAL) :
      screen(screen), type(type) {
  }

  Window(const std::shared_ptr<Window> &owner, WindowType type = WindowType::NORMAL) :
      screen(owner->screen), owner(owner), type(type) {
  }

  ~Window() {
    if (this->owner) {
      with_tree_locked([this] {
        this->owner->remove_owned_window(std::static_pointer_cast<Window>(shared_from_this()));
      });
    }
  }

  void init() override;

  friend class Screen;

protected:
  void add_notify() override;

  void paint_components(Graphics &g) override;

  void show() override;
  void hide() override;

  void dispatch_event_to_self(Event &e) {
    base::dispatch_event(e);
  }

public:
  WindowType get_type() const {
    return this->type;
  }

  void dispatch_event(Event &e) override;

  std::shared_ptr<Window> get_owner() const {
    return this->owner;
  }

  Screen* get_screen() const {
    return &this->screen;
  }

  bool is_displayable() const override {
    return true;
  }

  void paint(Graphics &g) override {
    validate();
    base::paint(g);
  }

  bool is_focus_cycle_root() const override {
    return true;
  }

  bool is_focused() const {
    return KeyboardFocusManager::get_focused_window().get() == this;
  }

  std::shared_ptr<Component> get_focus_owner() const;

  std::shared_ptr<Component> get_most_recent_focus_owner() const;

  bool is_focusable_window() const;

  bool get_focusable_window_state() const {
    return this->focusable_window_state;
  }
  void set_focusable_window_state(bool state);

  void set_always_on_top(bool value);
  bool is_always_on_top() const {
    return this->always_on_top;
  }

  void pack();
};

constexpr WindowEvent::WindowEvent(const std::shared_ptr<Window> &source_window, Type type, const std::shared_ptr<Window> &opposite_window) :
    Event(source_window, type), opposite_window(opposite_window) {
}

constexpr WindowEvent::WindowEvent(const std::shared_ptr<Window> &source, Type type) :
    WindowEvent(source, type, nullptr) {
}

constexpr std::shared_ptr<Window> WindowEvent::get_window() const {
  return std::dynamic_pointer_cast<Window>(this->source);
}

}
