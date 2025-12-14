#pragma once

#include <tui++/Char.h>
#include <tui++/event/InputEvent.h>

namespace tui {

class KeyEvent: public InputEvent {
public:
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

    VK_UNDEFINED = char32_t(0)
  };

  constexpr static Char CHAR_UNDEFINED { char32_t(0) };

  enum Type {
    KEY_TYPED = event_id_v<EventType::KEY, 0>,
    KEY_PRESSED = event_id_v<EventType::KEY, 1> ,
  };

private:
  union {
    KeyCode key_code;
    Char char_code;
  };

public:
  constexpr KeyEvent(const std::shared_ptr<Component> &source, Type type, KeyCode key_code, Modifiers modifiers, const EventClock::time_point &when = EventClock::now()) :
      InputEvent(source, type, modifiers, when), key_code(key_code) {
  }

  constexpr KeyEvent(const std::shared_ptr<Component> &source, const Char &c, Modifiers modifiers, const EventClock::time_point &when = EventClock::now()) :
      InputEvent(source, KEY_TYPED, modifiers, when), char_code(c) {
  }

public:
  constexpr KeyCode get_key_code() const {
    return this->id != KEY_TYPED ? this->key_code : VK_UNDEFINED;
  }

  constexpr Char get_key_char() const {
    return this->id == KEY_TYPED ? this->char_code : CHAR_UNDEFINED;
  }

  /**
   * Returns whether the key in this event is an "action" key.
   * Typically an action key does not fire a unicode character and is
   * not a modifier key.
   */
  constexpr bool is_action_key() const {
    switch (this->key_code) {
    case VK_HOME:
    case VK_END:
    case VK_PAGE_UP:
    case VK_PAGE_DOWN:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:

    case VK_F1:
    case VK_F2:
    case VK_F3:
    case VK_F4:
    case VK_F5:
    case VK_F6:
    case VK_F7:
    case VK_F8:
    case VK_F9:
    case VK_F10:
    case VK_F11:
    case VK_F12:
    case VK_INSERT:
      return true;

    default:
      return false;
    }
  }
};

std::string to_string(KeyEvent::KeyCode key_code);

}

#include <utility>

namespace std {

template<>
struct hash<tui::KeyEvent::KeyCode> {
  std::size_t operator()(tui::KeyEvent::KeyCode const &key) const noexcept {
    return std::hash<std::underlying_type_t<tui::KeyEvent::KeyCode>> { }(std::to_underlying(key));
  }
};

}
