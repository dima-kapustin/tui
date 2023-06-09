#pragma once

#include <memory>
#include <string>
#include <utility>
#include <functional>

namespace tui {

class Component;

struct BasicEvent {
  std::shared_ptr<Component> source;
};

struct InputEvent: BasicEvent {
  enum Modifiers {
    NONE_MASK = 0,
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
  enum KeyCode : char32_t {
    VK_TAB = '\t',
    VK_ENTER = '\r',
    VK_ESCAPE = '\x1b',
    VK_SPACE = ' ',
    VK_EXCLAMATION_MARK = '!',
    VK_DOUBLE_QUOTE = '"',
    VK_NUMBER_SIGN = '#',
    VK_DOLLAR = '$',
    VK_PERCENT = '%',
    VK_AMPERSAND = '&',
    VK_QUOTE = '\'',
    VK_LEFT_PARENTHESIS = '(',
    VK_RIGHT_PARENTHESIS = ')',
    VK_ASTERISK = '*',
    VK_PLUS = '+',
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

    VK_COLON = ':',
    VK_SEMICOLON = ';',
    VK_LESS = '<',
    VK_EQUALS = '=',
    VK_GREATER = '>',
    VK_QUESTION_MARK = '?',
    VK_AT = '@',

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
    VK_BACK_SLASH = '\\',
    VK_CLOSE_BRACKET = ']',
    VK_CARET = '^',
    VK_UNDERSCORE = '_',
    VK_BACK_QUOTE = '`',

    VK_LEFT_BRACE = '{',
    VK_PIPE = '|',
    VK_RIGHT_BRACE = '}',
    VK_DEAD_TILDE = '~',

    VK_BACK_SPACE = 127,

    // Special Keys - These overlap with C1 (128 - 159)
    VK_HOME = 128,
    VK_INSERT = 129,
    VK_DELETE = 130,
    VK_END = 131,
    VK_PAGE_UP = 132,
    VK_PAGE_DOWN = 133,
    // Empty
    // Empty
    // Empty
    // Empty
    VK_F1 = 138,
    VK_F2 = 139,
    VK_F3 = 140,
    VK_F4 = 141,
    VK_F5 = 142,
    // Empty
    VK_F6 = 144,
    VK_F7 = 145,
    VK_F8 = 146,
    VK_F9 = 147,
    VK_F10 = 148,
    // Empty
    VK_F11 = 150,
    VK_F12 = 151,

    VK_DOWN = 152,
    VK_UP = 153,
    VK_LEFT = 154,
    VK_RIGHT = 155,
    VK_BACK_TAB = 156,
  };

  KeyCode key_code;

  char get_key_char() const {
    constexpr auto low = short { ' ' };
    constexpr auto high = short { '~' };
    const auto value = static_cast<char32_t>(this->key_code);
    return (value < low or value > high) ? U'\0' : static_cast<char>(value);
  }

  char32_t get_key_char32() const {
    constexpr auto low = short { ' ' };
    constexpr auto mid = short { '~' };
    constexpr auto high = short { 160 };
    const auto value = static_cast<char32_t>(this->key_code);
    return (value < low or (value > mid and value < high)) ? U'\0' : value;
  }
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

namespace std {

template<>
struct hash<tui::KeyEvent::KeyCode> {
public:
  using argument_type = tui::KeyEvent::KeyCode;
  using result_type = std::size_t;

public:
  result_type operator()(argument_type const &key) const noexcept {
    return std::hash<std::underlying_type_t<argument_type>> { }(std::to_underlying(key));
  }
};

}
