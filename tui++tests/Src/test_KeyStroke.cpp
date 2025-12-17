#include <tui++/KeyStroke.h>

#include <cassert>

using namespace tui;

void test_KeyStroke() {
  constexpr auto a = KeyStroke { "typed A" };
  static_assert(a.get_key_char() == 'A');
  static_assert(a.get_key_event_type() == KeyEvent::KEY_TYPED);

  constexpr auto ctrl_a = KeyStroke { "ctrl A" };
  static_assert(ctrl_a.get_key_code() == KeyEvent::VK_A);
  static_assert(ctrl_a.get_key_char() == KeyEvent::CHAR_UNDEFINED);
  static_assert(ctrl_a.get_modifiers() == InputEvent::CTRL_DOWN);
  static_assert(ctrl_a.get_key_event_type() == KeyEvent::KEY_PRESSED);
}
