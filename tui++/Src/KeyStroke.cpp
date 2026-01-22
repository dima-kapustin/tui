#include <tui++/KeyStroke.h>

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

}
