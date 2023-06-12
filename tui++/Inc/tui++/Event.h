#pragma once

#include <tui++/event/KeyEvent.h>
#include <tui++/event/FocusEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/event/ActionEvent.h>
#include <tui++/event/InvocationEvent.h>

namespace tui {

std::string to_string(KeyEvent::KeyCode key_code);

class Event {
public:
  enum Type {
    UNDEFINED,
    KEY,
    MOUSE,
    ACTION,
    INVOCATION,
    FOCUS
  };

  const Type type = UNDEFINED;
  union {
    struct {
    } undefined { };
    KeyEvent key;
    MouseEvent mouse;
    ActionEvent action;
    InvocationEvent invocation;
    FocusEvent focus;
  };

  Event(const std::function<void()> &target) :
      type(INVOCATION), invocation { target } {
  }

  Event(const std::shared_ptr<Component> &source, KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) :
      type(Type::KEY), key { source, KeyEvent::KEY_PRESSED, key_code, modifiers } {
  }

  Event(const std::shared_ptr<Component> &source, const Char &c, InputEvent::Modifiers modifiers) :
      type(Type::KEY), key { source, c, modifiers } {
  }

  Event(const std::shared_ptr<Component> &source, MouseEvent::Type type, MouseEvent::Button button, InputEvent::Modifiers modifiers, int x, int y, int click_count_or_wheel_rotation, bool is_popup_trigger) :
      type(Type::MOUSE), mouse { source, type, button, modifiers, x, y, click_count_or_wheel_rotation, is_popup_trigger } {
  }

  Event(const std::shared_ptr<Component> &source, FocusEvent::Type type, bool temporary, const std::shared_ptr<Component> &opposite) :
      type(Type::FOCUS), focus { source, type, temporary, opposite } {
  }

  Event(const std::shared_ptr<Component> &source, const ActionKey &action_command, ActionEvent::Modifiers modifiers) :
      type(Type::ACTION), action { source, action_command, modifiers } {
  }

  ~Event() {
    switch (this->type) {
    case UNDEFINED:
      break;
    case KEY:
      this->key.~KeyEvent();
      break;
    case MOUSE:
      this->mouse.~MouseEvent();
      break;
    case ACTION:
      this->action.~ActionEvent();
      break;
    case INVOCATION:
      this->invocation.~InvocationEvent();
      break;
    case FOCUS:
      this->focus.~FocusEvent();
      break;
    }
  }
};

std::ostream& operator<<(std::ostream &os, const Event &event);

}

namespace std {

template<typename T>
constexpr const T& get(const tui::Event &event) noexcept (false) {
  auto type = tui::Event::UNDEFINED;
  const T *ptr = { };
  if constexpr (std::is_same_v<tui::KeyEvent, T>) {
    type == tui::Event::KEY;
    ptr = &event.key;
  } else if constexpr (std::is_same_v<tui::MouseEvent, T>) {
    type == tui::Event::MOUSE;
    ptr = &event.mouse;
  } else if constexpr (std::is_same_v<tui::FocusEvent, T>) {
    type == tui::Event::FOCUS;
    ptr = &event.focus;
  } else if constexpr (std::is_same_v<tui::ActionEvent, T>) {
    type == tui::Event::ACTION;
    ptr = &event.action;
  } else if constexpr (std::is_same_v<tui::InvocationEvent, T>) {
    type == tui::Event::INVOCATION;
    ptr = &event.invocation;
  }

  if (event.type == type) {
    return *ptr;
  } else {
    throw std::bad_variant_access();
  }
}

}
