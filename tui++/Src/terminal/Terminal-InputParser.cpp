#include <tui++/terminal/Terminal.h>

#include <tui++/util/utf-8.h>

namespace tui {

constexpr char STRING_TERMINATOR = '\\';

void Terminal::InputParser::parse_event() {
  switch (char c = consume(this->terminal.read_input_timeout)) {
  case '\x1b':
    parse_esc();
    break;

  case '\b':
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

  auto c = char32_t { };
  if (auto c_len = util::mb_to_c32(utf8.data(), std::size(utf8), &c); c_len > 0) {
    if (c < ' ') {
      new_key_event(KeyEvent::KeyCode(char(c + 0x40)), InputEvent::CTRL_DOWN);
    } else {
      new_key_event(c);
    }
  } else if (c_len == -1) {
    // TODO report eror: "Illegal byte sequence"
  } else if (c_len == -2) {
    // TODO report eror: "Incomplete byte sequence"
  }
}

void Terminal::InputParser::parse_esc() {
  // The byte after ESC is waited for with the read timeout: the terminal
  // writes each control sequence as one unit, so a byte that is not there
  // yet is simply lagging behind the ESC -- reading it with a zero timeout
  // would tear the sequence apart and leak the fragments as key events. A
  // lone ESC key still resolves to an ESCAPE after the timeout.
  switch (char c = consume(this->terminal.read_input_timeout)) {
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
      new_key_event(KeyEvent::KeyCode(c), InputEvent::ALT_DOWN);
    }
    break;
  }
}

void Terminal::InputParser::parse_ss3() {
  switch (consume(this->terminal.read_input_timeout)) {
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
  switch (consume(this->terminal.read_input_timeout)) {
  case '\x1b': // new esc sequence ?
    parse_esc();
    break;
  case STRING_TERMINATOR:
    break;
  }
}

void Terminal::InputParser::parse_csi() {
  // Wait for the byte after "ESC [" with the read timeout (see parse_esc:
  // sequence bytes lag behind their ESC under load and must not be read with
  // a zero timeout). A CSI that never completes is dropped.
  switch (char c = get(this->terminal.read_input_timeout)) {
  case 0:
    return;

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

  // Parameter bytes are waited for with the read timeout and a sequence that
  // never completes is dropped (a terminal writes each report in one unit, so
  // missing bytes mean the stream broke -- never emit fragments as keys).
  constexpr size_t MAX_PARAMS = 16;
  for (;;) {
    auto c = get(this->terminal.read_input_timeout);
    if (c == 0) {
      return;
    }

    if (c == ';') {
      consume();
      if (this->csi_params.size() < MAX_PARAMS) {
        this->csi_params.emplace_back(0);
      }
      continue;
    }

    if (c >= '0' and c <= '9') {
      auto value = 0u;
      do {
        value = value * 10 + unsigned(c - '0');
        consume(); // take the digit just peeked above
        c = get(this->terminal.read_input_timeout);
      } while (c >= '0' and c <= '9');
      this->csi_params.back() = value;
      if (c == 0) {
        return; // truncated after the digits
      }
      if (c == ';') {
        consume();
        if (this->csi_params.size() < MAX_PARAMS) {
          this->csi_params.emplace_back(0);
        }
        continue;
      }
    }

    // A final byte (the selector, e.g. 'M' for a mouse report): it was only
    // peeked, so parse_csi_selector() consumes it and dispatches.
    parse_csi_selector();
    return;
  }
}

std::optional<KeyEvent::KeyCode> Terminal::decode_csi_key(std::vector<unsigned> const &params, char selector) {
  switch (selector) {
  case 'A':
    return KeyEvent::VK_UP;
  case 'B':
    return KeyEvent::VK_DOWN;
  case 'C':
    return KeyEvent::VK_RIGHT;
  case 'D':
    return KeyEvent::VK_LEFT;
  case 'H':
    return KeyEvent::VK_HOME;
  case 'F':
    return KeyEvent::VK_END;
  case '~':
    // The numbered keys, ESC [ <code> ~: Insert, Delete, PageUp/Down and
    // F5..F12 (the unnumbered legacy keys of the table -- Find, Select,
    // Help, ... -- are not translated).
    switch (params.empty() ? 0 : params[0]) {
    case 2:
      return KeyEvent::VK_INSERT;
    case 3:
      return KeyEvent::VK_DELETE;
    case 5:
      return KeyEvent::VK_PAGE_UP;
    case 6:
      return KeyEvent::VK_PAGE_DOWN;
    case 15:
      return KeyEvent::VK_F5;
    case 17:
      return KeyEvent::VK_F6;
    case 18:
      return KeyEvent::VK_F7;
    case 19:
      return KeyEvent::VK_F8;
    case 20:
      return KeyEvent::VK_F9;
    case 21:
      return KeyEvent::VK_F10;
    case 23:
      return KeyEvent::VK_F11;
    case 24:
      return KeyEvent::VK_F12;
    default:
      return std::nullopt;
    }
  default:
    // The mouse reports ('M'/'m'), the 'R' cursor position report and any
    // other final byte are not keys.
    return std::nullopt;
  }
}

InputEvent::Modifiers Terminal::decode_csi_key_modifiers(std::vector<unsigned> const &params) {
  if (params.size() < 2) {
    return InputEvent::Modifiers::NONE;
  }

  switch (params[1]) {
  case 2:
    return InputEvent::SHIFT_DOWN;
  case 3:
    return InputEvent::ALT_DOWN;
  case 4:
    return InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN;
  case 5:
    return InputEvent::CTRL_DOWN;
  case 6:
    return InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN;
  case 7:
    return InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN;
  case 8:
    return InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN;
  case 9:
    return InputEvent::META_DOWN;
  case 10:
    return InputEvent::SHIFT_DOWN | InputEvent::META_DOWN;
  case 11:
    return InputEvent::ALT_DOWN | InputEvent::META_DOWN;
  case 12:
    return InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN;
  case 13:
    return InputEvent::CTRL_DOWN | InputEvent::META_DOWN;
  case 14:
    return InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::META_DOWN;
  case 15:
    return InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN;
  case 16:
    return InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN;
  }

  return InputEvent::NO_MODIFIERS;
}

void Terminal::InputParser::parse_csi_selector() {
  switch (char selector = consume()) {
  case 'M':
    new_mouse_event(true);
    break;
  case 'm':
    new_mouse_event(false);
    break;
  default:
    // The key selectors: the cursor keys 'A'..'D', Home/End 'H'/'F' and the
    // numbered '~' keys, with the xterm modifier parameter (CSI 1;<mod> A)
    // where the terminal sent one -- Ctrl+Up and Ctrl+Home/End arrive this
    // way. Anything else (the 'R' cursor position report, a '<' mouse
    // report's selector, ...) is not a key and is dropped.
    if (auto key_code = Terminal::decode_csi_key(this->csi_params, selector)) {
      new_key_event(key_code.value(), decode_csi_key_modifiers(this->csi_params));
    }
    break;
  }
}

void Terminal::InputParser::parse_osc() {
  switch (consume(this->terminal.read_input_timeout)) {
  case '\x1b': // new esc sequence ?
    parse_esc();
    break;
  case STRING_TERMINATOR:
    break;
  }
}

}
