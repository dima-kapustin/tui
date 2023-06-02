#include <tui++/Terminal.h>

namespace tui {

constexpr char STRING_TERMINATOR = '\\';

void Terminal::InputParser::parse(Input &input) {
  while (input) {
    switch (this->state) {
    case State::INIT:
      switch (char c = input.consume()) {
      case '\x1b':
        this->state = State::ESC;
        parse_esc(input);
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
        new_key_event(KeyEvent::VK_DELETE);
        break;

      case ' ':
        new_key_event(KeyEvent::VK_SPACE);
        break;

      default:
        new_key_event(KeyEvent::KeyCode(c));
        break;
      }
      break;

    case State::SKIP:
      if (input.consume() == '\x1b') {
        this->state = State::ESC;
        parse_esc(input);
      }
      break;

    case State::ESC:
      parse_esc(input);
      break;
    case State::SS3:
      parse_ss3(input);
      break;
    case State::DCS:
      parse_dcs(input);
      break;
    case State::CSI:
      parse_csi(input);
      break;
    case State::CSI_PARAMS:
      parse_csi_params(input);
      break;
    case State::CSI_SELECTOR:
      parse_csi_selector(input);
      break;
    case State::OSC:
      parse_osc(input);
      break;
    }
  }
}

void Terminal::InputParser::parse_esc(Input &input) {
  switch (char c = input.consume()) {
  case 'O':
    this->state = State::SS3;
    parse_ss3(input);
    break;
  case 'P':
    this->state = State::DCS;
    parse_dcs(input);
    break;
  case '[':
    this->state = State::CSI;
    reset_csi();
    parse_csi(input);
    break;
  case ']':
    this->state = State::OSC;
    parse_osc(input);
    break;
  case '\x1b':
    new_key_event(KeyEvent::VK_ESCAPE);
    break;

  default:
    if (c >= 0x40 and c <= 0x7E) {
      new_key_event(KeyEvent::KeyCode(c), InputEvent::ALT_MASK);
    }
    break;
  }
}

void Terminal::InputParser::parse_ss3(Input &input) {
  switch (input.consume()) {
  case '\x1b': // new esc sequence ?
    reset();
    parse_esc(input);
    break;
  case STRING_TERMINATOR:
    reset();
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

void Terminal::InputParser::parse_dcs(Input &input) {
  switch (input.consume()) {
  case '\x1b': // new esc sequence ?
    reset();
    parse_esc(input);
    break;
  case STRING_TERMINATOR:
    reset();
    break;
  }
}

void Terminal::InputParser::parse_csi(Input &input) {
  switch (input) {
  case 'A':
    input.consume();
    new_key_event(KeyEvent::VK_UP);
    return;
  case 'B':
    input.consume();
    new_key_event(KeyEvent::VK_DOWN);
    return;
  case 'C':
    input.consume();
    new_key_event(KeyEvent::VK_RIGHT);
    return;
  case 'D':
    input.consume();
    new_key_event(KeyEvent::VK_LEFT);
    return;
  case 'H':
    input.consume();
    new_key_event(KeyEvent::VK_HOME);
    return;
  case 'F':
    input.consume();
    new_key_event(KeyEvent::VK_END);
    return;
  case 'Z':
    input.consume();
    new_key_event(KeyEvent::VK_BACK_TAB);
    return;

  case '<':
    input.consume();
    this->state = State::CSI_PARAMS;
    this->csi_altered = true;
    parse_csi_params(input);
    break;

  default:
    this->state = State::CSI_PARAMS;
    parse_csi_params(input);
    break;
  }
}

void Terminal::InputParser::parse_csi_params(Input &input) {
  switch (input) {
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
      this->csi_params.back() = this->csi_params.back() * 10 + (input.consume() - '0');
    } while (std::isdigit(input));

    if (input != ';') {
      this->state = State::CSI_SELECTOR;
      parse_csi_selector(input);
      break;
    }
    [[fallthrough]];
  case ';':
    input.consume();
    this->csi_params.emplace_back(0);
    break;

  default:
    this->state = State::CSI_SELECTOR;
    parse_csi_selector(input);
    break;
  }
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

void Terminal::InputParser::parse_csi_selector(Input &input) {
  switch (input.consume()) {
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
    this->state = State::SKIP;
    break;
  }

  reset_csi();
}

void Terminal::InputParser::parse_osc(Input &input) {
  switch (input.consume()) {
  case '\x1b': // new esc sequence ?
    reset();
    parse_esc(input);
    break;
  case STRING_TERMINATOR:
    reset();
    break;
  }
}

}
