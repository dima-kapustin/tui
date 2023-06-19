#pragma once

#include <tui++/Component.h>
#include <tui++/event/WindowEvent.h>

namespace tui {

class Screen;
class KeyboardFocusManager;

class Window: public ComponentExtension<Component, WindowEvent> {
  using base = Component;

  Screen &screen;
  std::shared_ptr<Window> owner;

  Property<bool> focusable_window_state { this, "focusable_window_state", false };

  std::weak_ptr<Component> temporary_lost_component;

  struct {
    unsigned in_show :1 = false;
    unsigned before_first_show :1 = true;
    unsigned opened :1 = false;
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

  template<typename T, typename ... Args>
  void post_event(Args &&... args) {
    base::post_event<T, Window>(std::forward<Args>(args)...);
  }

  friend class KeyboardFocusManager;

protected:
  Window(Screen &screen) :
      screen(screen) {
  }

  friend class Screen;

protected:
  void paint_components(Graphics &g) override;

  virtual void show() override;
  virtual void hide() override;

public:
  Window(const std::shared_ptr<Window> &owner) :
      screen(owner->screen), owner(owner) {
  }

public:
  std::shared_ptr<Window> get_owner() const {
    return this->owner;
  }

  Screen* get_screen() const override {
    return &this->screen;
  }

  bool is_displayable() const override {
    return true;
  }

  void paint(Graphics &g) override {
    validate();
    base::paint(g);
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
};

constexpr WindowEvent::WindowEvent(const std::shared_ptr<Window> &source_window, Type type, const std::shared_ptr<Window> &opposite_window) :
    BasicEvent(source_window), type(type), opposite_window(opposite_window) {
}

constexpr WindowEvent::WindowEvent(const std::shared_ptr<Window> &source, Type type) :
    WindowEvent(source, type, nullptr) {
}

constexpr std::shared_ptr<Window> WindowEvent::get_window() const {
  return std::dynamic_pointer_cast<Window>(this->source);
}

}
