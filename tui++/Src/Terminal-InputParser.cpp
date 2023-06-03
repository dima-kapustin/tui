#include <tui++/Terminal.h>

namespace tui {

char32_t mb_to_u32(std::array<char, 4> bytes);

constexpr char STRING_TERMINATOR = '\\';

void Terminal::InputParser::parse_event() {
  switch (char c = consume(read_terminal_input_timeout)) {
  case '\x1b':
    parse_esc();
    break;

  case '\x8':
    new_key_event(KeyEvent::VK_BACK_SPACE);
    break;

  case '\r':
  case '\n':
    new_key_event(KeyEvent::VK_ENTER);
    break;

  case '\t':
    new_key_event(KeyEvent::VK_TAB);
    break;

  case '\x7f':
    new_key_event(KeyEvent::VK_BACK_SPACE);
    break;

  case ' ':
    new_key_event(KeyEvent::VK_SPACE);
    break;

  case 0:
    break;

  default:
    parse_utf8(c);
    break;
  }
}

void Terminal::InputParser::parse_utf8(char first_byte) {
  // utf8 initial bytes
  constexpr uint8_t mask_1 = 0b11000000;
  constexpr uint8_t mask_2 = 0b11100000;
  constexpr uint8_t mask_3 = 0b11110000;

  auto matches = [](char value, char mask) -> bool {
    return (value & mask) == mask;
  };

  size_t bytes_left_to_read = matches(first_byte, mask_1);
  bytes_left_to_read += matches(first_byte, mask_2);
  bytes_left_to_read += matches(first_byte, mask_3);

  std::array<char, 4> utf8 = { first_byte };
  for (size_t i = 1; i <= bytes_left_to_read; ++i) {
    utf8[i] = consume();
  }
  new_key_event(KeyEvent::KeyCode(mb_to_u32(utf8)));
}

void Terminal::InputParser::parse_esc() {
  switch (char c = consume()) {
  case 'O':
    parse_ss3();
    break;
  case 'P':
    parse_dcs();
    break;
  case '[':
    parse_csi();
    break;
  case ']':
    parse_osc();
    break;

  case '\x1b':
    new_key_event(KeyEvent::VK_ESCAPE);
    parse_esc();
    break;

  case 0:
    new_key_event(KeyEvent::VK_ESCAPE);
    break;

  default:
    if (c >= 0x40 and c <= 0x7E) {
      new_key_event(KeyEvent::KeyCode(c), InputEvent::ALT_MASK);
    }
    break;
  }
}

void Terminal::InputParser::parse_ss3() {
  switch (consume()) {
  case '\x1b': // new esc sequence ?
    parse_esc();
    break;
  case STRING_TERMINATOR:
    break;
  case 'A':
    new_key_event(KeyEvent::VK_UP);
    break;
  case 'B':
    new_key_event(KeyEvent::VK_DOWN);
    break;
  case 'C':
    new_key_event(KeyEvent::VK_RIGHT);
    break;
  case 'D':
    new_key_event(KeyEvent::VK_LEFT);
    break;
  case 'H':
    new_key_event(KeyEvent::VK_HOME);
    break;
  case 'F':
    new_key_event(KeyEvent::VK_END);
    break;
  case 'P':
    new_key_event(KeyEvent::VK_F1);
    break;
  case 'Q':
    new_key_event(KeyEvent::VK_F2);
    break;
  case 'R':
    new_key_event(KeyEvent::VK_F3);
    break;
  case 'S':
    new_key_event(KeyEvent::VK_F4);
    break;
  }
}

void Terminal::InputParser::parse_dcs() {
  switch (consume()) {
  case '\x1b': // new esc sequence ?
    parse_esc();
    break;
  case STRING_TERMINATOR:
    break;
  }
}

void Terminal::InputParser::parse_csi() {
  switch (get()) {
  case 'A':
    consume();
    new_key_event(KeyEvent::VK_UP);
    return;
  case 'B':
    consume();
    new_key_event(KeyEvent::VK_DOWN);
    return;
  case 'C':
    consume();
    new_key_event(KeyEvent::VK_RIGHT);
    return;
  case 'D':
    consume();
    new_key_event(KeyEvent::VK_LEFT);
    return;
  case 'H':
    consume();
    new_key_event(KeyEvent::VK_HOME);
    return;
  case 'F':
    consume();
    new_key_event(KeyEvent::VK_END);
    return;
  case 'Z':
    consume();
    new_key_event(KeyEvent::VK_BACK_TAB);
    return;

  case '<':
    consume();
    this->csi_altered = true;
    parse_csi_params();
    break;

  default:
    this->csi_altered = false;
    parse_csi_params();
    break;
  }
}

void Terminal::InputParser::parse_csi_params() {
  this->csi_params.resize(1);
  this->csi_params[0] = 0;

  do {
    switch (char c = get()) {
    case 0:
      return;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      do {
        this->csi_params.back() = this->csi_params.back() * 10 + (consume() - '0');
      } while (std::isdigit(c = get()));

      if (c != ';') {
        parse_csi_selector();
        return;
      }
      [[fallthrough]];
    case ';':
      consume();
      this->csi_params.emplace_back(0);
      break;

    default:
      parse_csi_selector();
      return;
    }
  } while (true);
}

constexpr InputEvent::Modifiers operator|(InputEvent::Modifiers a, InputEvent::Modifiers b) {
  return InputEvent::Modifiers(std::underlying_type_t<InputEvent::Modifiers>(a) | std::underlying_type_t<InputEvent::Modifiers>(b));
}

static InputEvent::Modifiers parse_modifiers(const std::vector<unsigned> &args) {
  if (args.size() < 2) {
    return InputEvent::Modifiers::NONE_MASK;
  }

  using Modifiers = InputEvent::Modifiers;

  switch (args[1]) {
  case 2:
    return Modifiers::SHIFT_MASK;
  case 3:
    return Modifiers::ALT_MASK;
  case 4:
    return Modifiers::SHIFT_MASK | Modifiers::ALT_MASK;
  case 5:
    return Modifiers::CTRL_MASK;
  case 6:
    return Modifiers::SHIFT_MASK | Modifiers::CTRL_MASK;
  case 7:
    return Modifiers::CTRL_MASK | Modifiers::ALT_MASK;
  case 8:
    return Modifiers::SHIFT_MASK | Modifiers::CTRL_MASK | Modifiers::ALT_MASK;
  case 9:
    return Modifiers::META_MASK;
  case 10:
    return Modifiers::SHIFT_MASK | Modifiers::META_MASK;
  case 11:
    return Modifiers::ALT_MASK | Modifiers::META_MASK;
  case 12:
    return Modifiers::SHIFT_MASK | Modifiers::ALT_MASK | Modifiers::META_MASK;
  case 13:
    return Modifiers::CTRL_MASK | Modifiers::META_MASK;
  case 14:
    return Modifiers::SHIFT_MASK | Modifiers::CTRL_MASK | Modifiers::META_MASK;
  case 15:
    return Modifiers::CTRL_MASK | Modifiers::ALT_MASK | Modifiers::META_MASK;
  case 16:
    return Modifiers::SHIFT_MASK | Modifiers::CTRL_MASK | Modifiers::ALT_MASK | Modifiers::META_MASK;
  }

  return InputEvent::Modifiers::NONE_MASK;
}

void Terminal::InputParser::parse_csi_selector() {
  switch (consume()) {
  case 'M':
    new_mouse_event(true);
    break;
  case 'm':
    new_mouse_event(false);
    break;
  case 'A':
    new_key_event(KeyEvent::VK_UP, parse_modifiers(this->csi_params));
    break;
  case 'B':
    new_key_event(KeyEvent::VK_DOWN, parse_modifiers(this->csi_params));
    break;
  case 'C':
    new_key_event(KeyEvent::VK_RIGHT, parse_modifiers(this->csi_params));
    break;
  case 'D':
    new_key_event(KeyEvent::VK_LEFT, parse_modifiers(this->csi_params));
    break;
  case 'R':
    break;

  case '~':
    switch (this->csi_params[0]) {
    case 1:
      //new_key_event(KeyEvent::VK_FIND);
      break;
    case 2:
      new_key_event(KeyEvent::VK_INSERT);
      break;
    case 3:
      new_key_event(KeyEvent::VK_DELETE);
      break;
    case 4:
      //new_key_event(KeyEvent::VK_SELECT);
      break;
    case 5:
      new_key_event(KeyEvent::VK_PAGE_UP);
      break;
    case 6:
      new_key_event(KeyEvent::VK_PAGE_DOWN);
      break;
    case 15:
      new_key_event(KeyEvent::VK_F5);
      break;
    case 17:
      new_key_event(KeyEvent::VK_F6);
      break;
    case 18:
      new_key_event(KeyEvent::VK_F7);
      break;
    case 19:
      new_key_event(KeyEvent::VK_F8);
      break;
    case 20:
      new_key_event(KeyEvent::VK_F9);
      break;
    case 21:
      new_key_event(KeyEvent::VK_F10);
      break;
    case 23:
      new_key_event(KeyEvent::VK_F11);
      break;
    case 24:
      new_key_event(KeyEvent::VK_F12);
      break;
    case 28:
      //new_key_event(KeyEvent::VK_HELP);
      break;
    case 29:
      //new_key_event(KeyEvent::VK_MENU);
      break;
    }
    break;

  default:
    break;
  }
}

void Terminal::InputParser::parse_osc() {
  switch (consume()) {
  case '\x1b': // new esc sequence ?
    parse_esc();
    break;
  case STRING_TERMINATOR:
    break;
  }
}

}
