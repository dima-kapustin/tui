#pragma once

#include <tui++/Event.h>

namespace tui {

class KeyStroke {
  InputEvent::Modifiers modifiers;
  KeyEvent::KeyCode key_code;

};

}
