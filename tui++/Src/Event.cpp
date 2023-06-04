#include <tui++/Event.h>
#include <tui++/util/utf-8.h>

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
      buf += "Down"sv;
      break;
    case KeyEvent::VK_UP:
      buf += "Up"sv;
      break;
    case KeyEvent::VK_LEFT:
      buf += "Left"sv;
      break;
    case KeyEvent::VK_RIGHT:
      buf += "Right"sv;
      break;
    case KeyEvent::VK_HOME:
      buf += "Home"sv;
      break;
    case KeyEvent::VK_BACK_SPACE:
      buf += "BackSpace"sv;
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
      buf += std::format("F{}", c);
      break;
    }
    case KeyEvent::VK_DELETE:
      buf += "Delete"sv;
      break;
    case KeyEvent::VK_INSERT:
      buf += "Insert"sv;
      break;
    case KeyEvent::VK_PAGE_DOWN:
      buf += "PageDown"sv;
      break;
    case KeyEvent::VK_PAGE_UP:
      buf += "PageUp"sv;
      break;
    case KeyEvent::VK_ENTER:
      buf += "Enter"sv;
      break;
    case KeyEvent::VK_BACK_TAB:
      buf += "BackTab"sv;
      break;
    case KeyEvent::VK_END:
      buf += "End"sv;
      break;
    default:
      buf += util::u32_to_mb(key_code);
    }
  }

  return buf;
}

std::ostream& operator<<(std::ostream &os, const Event &event) {
  switch (event.type) {
  case Event::KEY:
    os << "Key Pressed: ";
    if (event.key.modifiers & KeyEvent::ALT_MASK) {
      os << "Alt+";
    }
    if (event.key.modifiers & KeyEvent::SHIFT_MASK) {
      os << "Shift+";
    }
    if (event.key.modifiers & KeyEvent::CTRL_MASK) {
      os << "Ctrl+";
    }
    os << to_string(event.key.key_code);
    break;

  case Event::MOUSE:
    os << "Mouse ";

    if (event.mouse.type == MouseEvent::MOUSE_MOVED) {
      os << "Moved with ";
    } else if (event.mouse.type == MouseEvent::MOUSE_DRAGGED) {
      os << "Dragged with ";
    }

    if (event.mouse.type != MouseEvent::MOUSE_WHEEL) {
      switch (event.mouse.button) {
      case MouseEvent::LEFT_BUTTON:
        os << "Left Button";
        break;
      case MouseEvent::MIDDLE_BUTTON:
        os << "Middle Button";
        break;
      case MouseEvent::RIGHT_BUTTON:
        os << "Right Button";
        break;
      case MouseEvent::NO_BUTTON:
        os << "No Button";
        break;
      }
    }

    switch (event.mouse.type) {
    case MouseEvent::MOUSE_PRESSED:
      os << " Pressed";
      break;
    case MouseEvent::MOUSE_RELEASED:
      os << " Released";
      break;
    case MouseEvent::MOUSE_CLICKED:
      if (event.mouse.click_count == 2) {
        os << " Double Clicked";
      } else {
        os << " Clicked";
      }
      break;

    case MouseEvent::MOUSE_WHEEL:
      os << " Wheel scrolled by " << event.mouse.weel_rotation;
      break;

    default:
      break;
    }

    if (event.mouse.modifiers) {
      os << " with ";
      if (event.mouse.modifiers & InputEvent::ALT_MASK) {
        os << "Alt";
      }
      if (event.mouse.modifiers & InputEvent::SHIFT_MASK) {
        os << "Shift";
      }
      if (event.mouse.modifiers & InputEvent::CTRL_MASK) {
        os << "Ctrl";
      }
      if (event.mouse.modifiers & InputEvent::META_MASK) {
        os << "Meta";
      }
    }

    os << " at " << event.mouse.x << "," << event.mouse.y;
    break;

  case Event::INVOCATION:
    os << "Invocation";
    break;

  case Event::FOCUS:
    switch (event.focus.type) {
    case FocusEvent::FOCUS_GAINED:
      os << "Focus Gained";
      break;
    case FocusEvent::FOCUS_LOST:
      os << "Focus Lost";
      break;
    }
    os << (event.focus.temporary ? " temporary" : "");
    break;

  case Event::UNDEFINED:
    os << "UNDEFINED";
    break;
  }
  return os;
}

}
