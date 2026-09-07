#include <tui++/KeyStroke.h>

#include <tui++/util/utf-8.h>

#include <string_view>

namespace tui {

KeyStroke KeyStroke::parse(u8string const &str) {
  using namespace std::string_literals;
  using namespace std::string_view_literals;

  auto throw_invalid_format = [&str] {
    throw std::runtime_error("Invalid KeyStroke: "s + str);
  };

  auto typed = false;
  auto char_index = size_t { };
  auto modifiers = InputEvent::Modifiers::NONE;
  auto key_code = KeyEvent::VK_UNDEFINED;
  while (char_index < str.size()) {
    if (auto token = util::next_token(str, char_index, ' '); not token.empty()) { // multiple spaces ?
      if (typed) {
        if (char_index == str.size()) {
          auto cp = char32_t { };
          if (auto cp_len = util::mb_to_c32(token, &cp); cp_len > 0 and token.length() == size_t(cp_len)) {
            return {cp, modifiers};
          }
        }
        throw_invalid_format();
      }

      if (token == "typed"sv) {
        typed = true;
      } else if (token == "ctrl"sv or token == "control"sv) {
        modifiers |= InputEvent::CTRL_DOWN;
      } else if (token == "shift"sv) {
        modifiers |= InputEvent::SHIFT_DOWN;
      } else if (token == "alt"sv) {
        modifiers |= InputEvent::ALT_DOWN;
      } else {
        if (char_index == str.size()) {
          auto cp = char32_t { };
          if (auto cp_len = util::mb_to_c32(token, &cp); cp_len > 0 and token.length() == size_t(cp_len)) {
            key_code = KeyEvent::KeyCode(cp);
            break;
          }
        }
        throw_invalid_format();
      }
    }
  }

  return {key_code, modifiers};
}

std::string to_string(KeyStroke const &key_stroke) {
  auto out = std::string { };
  auto add_part = [&out](std::string_view part) {
    if (not out.empty()) {
      out += '+';
    }
    out += part;
  };
  auto add_char32 = [&out, &add_part](char32_t code) {
    // Encodes `code` as UTF-8; the parser and the Char class use UTF-32
    // internally, and terminals (and the metrics) want UTF-8.
    char bytes[4];
    auto n = util::c32_to_mb(code, bytes);
    add_part(std::string_view { bytes, n });
  };

  auto modifiers = key_stroke.get_modifiers();
  if (modifiers & InputEvent::CTRL_DOWN) {
    add_part("Ctrl");
  }
  if (modifiers & InputEvent::ALT_DOWN) {
    add_part("Alt");
  }
  if (modifiers & InputEvent::SHIFT_DOWN) {
    add_part("Shift");
  }
  if (modifiers & InputEvent::META_DOWN) {
    add_part("Meta");
  }

  if (key_stroke.get_event_type() == KeyEvent::KEY_TYPED) {
    // A typed stroke ("typed A"): its character, letters uppercased.
    auto code = key_stroke.get_key_char().get_code();
    if (code >= char32_t('a') and code <= char32_t('z')) {
      code = code - char32_t('a') + char32_t('A');
    }
    add_char32(code);
  } else {
    // A pressed stroke ("ctrl Z", "ctrl insert"): the key's name; the
    // single letters the parser produces come out uppercase.
    auto key_code = key_stroke.get_key_code();
    auto name = to_string(key_code);
    if (key_code < 0x80 and name.size() == 1 and name[0] >= 'a' and name[0] <= 'z') {
      name[0] = char(name[0] - 'a' + 'A');
    }
    add_part(name);
  }
  return out;
}

}
