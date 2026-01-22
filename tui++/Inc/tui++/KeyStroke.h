#pragma once

#include <tui++/Event.h>
#include <tui++/util/utf-8.h>

namespace tui {

class KeyStroke {
  Char key_char = '\0';
  KeyEvent::KeyCode key_code;
  InputEvent::Modifiers modifiers;

private:
  static KeyStroke parse(const u8string &str);

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

  KeyStroke(u8string const &str) :
      KeyStroke(parse(str)) {
  }

  constexpr KeyStroke(KeyEvent const &event) :
      key_char(event.get_key_char()), key_code(event.get_key_code()), modifiers(event.modifiers) {
  }

  constexpr KeyStroke(KeyStroke const&) = default;
  constexpr KeyStroke(KeyStroke&&) = default;

  constexpr KeyStroke& operator=(KeyStroke const&) = default;
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

  constexpr bool operator==(const KeyStroke &other) const = default;
};

}

namespace std {

template<>
struct hash<tui::KeyStroke> {
  std::size_t operator()(tui::KeyStroke const &key_stroke) const noexcept {
    return (key_stroke.get_key_char().get_code() + 1) * (key_stroke.get_key_code() + 1) * (key_stroke.get_modifiers() + 1) * 2;
  }
};

}

#include <memory>
#include <unordered_set>

namespace tui {

template<typename ... KeyStrokes>
constexpr std::shared_ptr<std::unordered_set<KeyStroke>> make_shared_keystroke_set(KeyStrokes &&... keyStrokes) {
  auto set = std::make_shared<std::unordered_set<KeyStroke>>();
  (set->emplace(std::forward<KeyStrokes>(keyStrokes)), ...);
  return set;
}

}
