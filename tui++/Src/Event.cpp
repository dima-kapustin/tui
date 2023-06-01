#include <tui++/Event.h>

#include <format>

using namespace std::string_view_literals;

namespace tui {

std::string to_string(KeyEvent::KeyCode key_code) {
  std::string buf;
  buf.reserve(128);

  if (key_code < KeyEvent::VK_SPACE) {
    buf += '^';
    buf += (char) (key_code + 0x40);
  } else if (key_code == KeyEvent::VK_SPACE) {
    buf += "SPACE"sv;
  } else if (key_code < 0x7f) {
    buf += (char) key_code;
  } else {
    switch (key_code) {
    case KeyEvent::VK_DOWN:
      buf += "VK_DOWN"sv;
      break;
    case KeyEvent::VK_UP:
      buf += "VK_UP"sv;
      break;
    case KeyEvent::VK_LEFT:
      buf += "VK_LEFT"sv;
      break;
    case KeyEvent::VK_RIGHT:
      buf += "VK_RIGHT"sv;
      break;
    case KeyEvent::VK_HOME:
      buf += "VK_HOME"sv;
      break;
    case KeyEvent::VK_BACK_SPACE:
      buf += "VK_BACK_SPACE"sv;
      break;
    case KeyEvent::VK_F1:
    case KeyEvent::VK_F2:
    case KeyEvent::VK_F3:
    case KeyEvent::VK_F4:
    case KeyEvent::VK_F5:
    case KeyEvent::VK_F6:
    case KeyEvent::VK_F7:
    case KeyEvent::VK_F8:
    case KeyEvent::VK_F9:
    case KeyEvent::VK_F10:
    case KeyEvent::VK_F11:
    case KeyEvent::VK_F12: {
      auto c = 1 + key_code - KeyEvent::VK_F1;
      buf += std::format("VK_F{}", c);
      break;
    }
    case KeyEvent::VK_DELETE:
      buf += "VK_DELETE"sv;
      break;
    case KeyEvent::VK_INSERT:
      buf += "VK_INSERT"sv;
      break;
    case KeyEvent::VK_PAGE_DOWN:
      buf += "VK_PAGE_DOWN"sv;
      break;
    case KeyEvent::VK_PAGE_UP:
      buf += "VK_PAGE_UP"sv;
      break;
    case KeyEvent::VK_ENTER:
      buf += "VK_ENTER"sv;
      break;
    case KeyEvent::VK_BACK_TAB:
      buf += "VK_BACK_TAB"sv;
      break;
    case KeyEvent::VK_END:
      buf += "VK_END"sv;
      break;
    default:
      buf += "UNKNOWN"sv;
    }
  }

  return buf;
}

}
