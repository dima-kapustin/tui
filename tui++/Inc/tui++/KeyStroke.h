#pragma once

#include <tui++/Event.h>
#include <tui++/util/utf-8.h>

namespace tui {

class KeyStroke {
  Char key_char;
  KeyEvent::KeyCode key_code;
  InputEvent::Modifiers modifiers;

private:
  constexpr KeyStroke parse(const u8string &str) {
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    auto throw_invalid_format = [&str] {
      throw std::runtime_error("Invalid KeyStroke: "s + str);
    };

    auto typed = false;
    auto char_index = size_t { 0 };
    auto modifiers = InputEvent::Modifiers::NONE;
    auto key_code = KeyEvent::VK_UNDEFINED;
    while (char_index < str.size()) {
      auto token = util::next_token(str, char_index, ' ');
      if (not token.empty()) { // multiple spaces ?
        if (typed) {
          if (char_index == str.size()) {
            auto cp = char32_t { 0 };
            auto cp_len = util::mb_to_c32(token, &cp);
            if (cp_len > 0 and token.length() == size_t(cp_len)) {
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
            auto cp = char32_t { 0 };
            auto cp_len = util::mb_to_c32(token, &cp);
            if (cp_len > 0 and token.length() == size_t(cp_len)) {
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

public:
  constexpr KeyStroke() = default;

  constexpr KeyStroke(char c) :
      key_char(c), key_code(KeyEvent::VK_UNDEFINED), modifiers(InputEvent::NO_MODIFIERS) {
  }

  constexpr KeyStroke(const Char &key_char, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) :
      key_char(key_char), key_code(KeyEvent::VK_UNDEFINED), modifiers(modifiers) {
  }

  constexpr KeyStroke(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) :
      key_char(KeyEvent::CHAR_UNDEFINED), key_code(key_code), modifiers(modifiers) {
  }

  constexpr KeyStroke(const u8string &str) :
      KeyStroke(parse(str)) {
  }

  constexpr KeyStroke(const KeyEvent &event) :
      modifiers(event.modifiers) {
    switch (event.type) {
    case KeyEvent::KEY_TYPED:
      this->key_char = event.get_key_char();
      this->key_code = KeyEvent::VK_UNDEFINED;
      break;
    case KeyEvent::KEY_PRESSED:
      this->key_char = KeyEvent::CHAR_UNDEFINED;
      this->key_code = event.get_key_code();
      break;
    }
  }

  constexpr KeyStroke(const KeyStroke&) = default;
  constexpr KeyStroke(KeyStroke&&) = default;

  constexpr KeyStroke& operator=(const KeyStroke&) = default;
  constexpr KeyStroke& operator=(KeyStroke&&) = default;

public:
  constexpr const Char& get_key_char() const {
    return this->key_char;
  }

  constexpr KeyEvent::KeyCode get_key_code() const {
    return this->key_code;
  }

  constexpr InputEvent::Modifiers get_modifiers() const {
    return this->modifiers;
  }

  constexpr KeyEvent::Type get_key_event_type() const {
    if (this->key_code == KeyEvent::VK_UNDEFINED) {
      return KeyEvent::KEY_TYPED;
    }
    return KeyEvent::KEY_PRESSED;
  }

  constexpr bool operator==(const KeyStroke &other) const {
    return this->key_char == other.key_char and this->key_code == other.key_code and this->modifiers == other.modifiers;
  }
};

}

namespace std {

template<>
struct hash<tui::KeyStroke> {
public:
  using argument_type = tui::KeyStroke;
  using result_type = std::size_t;

public:
  result_type operator()(argument_type const &key_stroke) const noexcept {
    return (key_stroke.get_key_char().get_code() + 1) * (key_stroke.get_key_code() + 1) * (key_stroke.get_modifiers() + 1) * 2;
  }
};

}

#include <memory>

namespace tui {

template<typename ... KeyStrokes>
constexpr std::shared_ptr<std::unordered_set<KeyStroke>> make_shared_keystroke_set(KeyStrokes &&... keyStrokes) {
  auto set = std::make_shared<std::unordered_set<KeyStroke>>();
  (set->emplace(std::forward<KeyStrokes>(keyStrokes)), ...);
  return set;
}

}
