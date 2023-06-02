#pragma once

#include <memory>
#include <string>
#include <functional>

namespace tui {

class Component;

struct BasicEvent {
  std::shared_ptr<Component> source;
};

struct InputEvent: BasicEvent {
  enum Modifiers {
    NONE = 0,
    /** The shift key modifier constant */
    SHIFT_MASK = 1 << 0,

    /** The control key modifier constant */
    CTRL_MASK = 1 << 1,

    /** The alt key modifier constant */
    ALT_MASK = 1 << 2,

    META_MASK = 1 << 4
  };

  Modifiers modifiers;
};

struct MouseEvent: InputEvent {
  enum Type {
    MOUSE_PRESSED,
    MOUSE_RELEASED,
    MOUSE_CLICKED,
    MOUSE_WHEEL,
    MOUSE_MOVED,
    MOUSE_DRAGGED
  };

  enum Button {
    LEFT_BUTTON = 0,
    MIDDLE_BUTTON = 1,
    RIGHT_BUTTON = 2,
    NO_BUTTON = 3
  };

  Type type;
  Button button = NO_BUTTON;
  int x, y;

  union {
    unsigned click_count;
    // The number of "clicks" the mouse wheel was rotated.
    // Negative values denote mouse wheel rotation up/away from the user, and positive values denote mouse wheel was rotation down/towards the user.
    int weel_rotation;
  };
  bool is_popup_trigger;
};

struct KeyEvent: InputEvent {
  enum KeyCode {
    VK_ESCAPE = '\x1b',
    VK_SPACE = ' ',

    VK_TAB = '\t',
    VK_BACK_TAB = 0xe000,

    VK_DOWN = 0xe001,
    VK_UP = 0xe002,
    VK_LEFT = 0xe003,
    VK_RIGHT = 0xe004,
    VK_HOME = 0xe005,
    VK_END = 0xe006,
    VK_BACK_SPACE = 0xe007,

    VK_F1 = 0xe008,
    VK_F2 = 0xe009,
    VK_F3 = 0xe00a,
    VK_F4 = 0xe00b,
    VK_F5 = 0xe00c,
    VK_F6 = 0xe00d,
    VK_F7 = 0xe00e,
    VK_F8 = 0xe00f,
    VK_F9 = 0xe010,
    VK_F10 = 0xe011,
    VK_F11 = 0xe012,
    VK_F12 = 0xe013,

    VK_DELETE = 0xe01a,
    VK_INSERT = 0xe01b,
    VK_PAGE_DOWN = 0xe01c,
    VK_PAGE_UP = 0xe01d,

    VK_ENTER = 0xe01e,

    VK_COMMA = ',',
    VK_MINUS = '-',
    VK_PERIOD = '.',
    VK_SLASH = '/',

    VK_0 = '0',
    VK_1 = '1',
    VK_2 = '2',
    VK_3 = '3',
    VK_4 = '4',
    VK_5 = '5',
    VK_6 = '6',
    VK_7 = '7',
    VK_8 = '8',
    VK_9 = '9',

    VK_SEMICOLON = ';',
    VK_EQUALS = '=',

    VK_A = 'A',
    VK_B = 'B',
    VK_C = 'C',
    VK_D = 'D',
    VK_E = 'E',
    VK_F = 'F',
    VK_G = 'G',
    VK_H = 'H',
    VK_I = 'I',
    VK_J = 'J',
    VK_K = 'K',
    VK_L = 'L',
    VK_M = 'M',
    VK_N = 'N',
    VK_O = 'O',
    VK_P = 'P',
    VK_Q = 'Q',
    VK_R = 'R',
    VK_S = 'S',
    VK_T = 'T',
    VK_U = 'U',
    VK_V = 'V',
    VK_W = 'W',
    VK_X = 'X',
    VK_Y = 'Y',
    VK_Z = 'Z',
    VK_OPEN_BRACKET = '[',
    VK_CLOSE_BRACKET = ']',
    VK_OPEN_PAREN = '(',
    VK_CLOSE_PAREN = ')',
  };

  KeyCode key_code;
};

std::string to_string(KeyEvent::KeyCode key_code);

struct InvocationEvent {
  std::function<void()> target;
};

struct FocusEvent: BasicEvent {
  enum Type {
    FOCUS_LOST,
    FOCUS_GAINED
  };

  Type type;
  bool temporary = false;
  std::shared_ptr<Component> opposite;
};

class Event {
public:
  enum Type {
    UNDEFINED,
    KEY,
    MOUSE,
    INVOCATION,
    FOCUS
  };

  const Type type = UNDEFINED;
  union {
    struct {
    } undefined { };
    KeyEvent key;
    MouseEvent mouse;
    InvocationEvent invocation;
    FocusEvent focus;
  };

  Event(const std::function<void()> &target) :
      type(INVOCATION), invocation { target } {
  }

  Event(const std::shared_ptr<Component> &source, KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) :
      type(Type::KEY), key { source, modifiers, key_code } {
  }

  Event(const std::shared_ptr<Component> &source, MouseEvent::Type type, MouseEvent::Button button, InputEvent::Modifiers modifiers, int x, int y, int click_count_or_wheel_rotation, bool is_popup_trigger) :
      type(Type::MOUSE), mouse { source, modifiers, type, button, x, y, (unsigned) click_count_or_wheel_rotation, is_popup_trigger } {
  }

  Event(const std::shared_ptr<Component> &source, FocusEvent::Type type, bool temporary, const std::shared_ptr<Component> &opposite) :
      type(Type::FOCUS), focus { source, type, temporary, opposite } {
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
