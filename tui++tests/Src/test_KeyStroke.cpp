#include <tui++/KeyStroke.h>

#include <cassert>

using namespace tui;

void test_KeyStroke() {
  auto a = KeyStroke { "typed A" };
  assert(a.get_key_char() == 'A');
  assert(a.get_event_type() == KeyEvent::KEY_TYPED);

  auto ctrl_a = KeyStroke { "ctrl A" };
  assert(ctrl_a.get_key_code() == KeyEvent::VK_A);
  assert(ctrl_a.get_key_char() == KeyEvent::CHAR_UNDEFINED);
  assert(ctrl_a.get_modifiers() == InputEvent::CTRL_DOWN);
  assert(ctrl_a.get_event_type() == KeyEvent::KEY_PRESSED);
}
