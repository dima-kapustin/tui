#pragma once

#include <tui++/Component.h>
#include <tui++/event/WindowEvent.h>

namespace tui {

class Screen;

class Window: public ComponentExtension<Component, WindowEvent> {
  using base = Component;

  Screen &screen;
  std::shared_ptr<Window> owner;

  Property<bool> focusable_window_state { this, "focusable_window_state", false };

protected:
  void paint_components(Graphics &g) override;

  Window(Screen &screen) :
      screen(screen) {
  }

  friend class Screen;
public:
  Window(const std::shared_ptr<Window> &owner) :
      screen(owner->screen), owner(owner) {
  }

public:
  std::shared_ptr<Window> get_owner() const {
    // TODO
    return {};
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

  bool is_focusable_window() const;

  bool get_focusable_window_state() const {
    return this->focusable_window_state;
  }

  void set_focusable_window_state(bool state);
};

constexpr WindowEvent::WindowEvent(const std::shared_ptr<Window> &source_window, Type type, const std::shared_ptr<Window> &opposite_window) :
    BasicEvent(source_window), type(type), opposite_window(opposite_window) {
}

constexpr std::shared_ptr<Window> WindowEvent::get_window() const {
  return std::dynamic_pointer_cast<Window>(this->source);
}


}
