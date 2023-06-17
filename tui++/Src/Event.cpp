#include <tui++/Event.h>
#include <tui++/util/utf-8.h>

#include <format>

using namespace std::string_view_literals;

namespace tui {

std::string to_string(KeyEvent::KeyCode key_code) {
  std::string buf;
  buf.reserve(128);

  if (key_code < KeyEvent::VK_SPACE) {
    switch (key_code) {
    case KeyEvent::VK_TAB:
      buf += "Tab"sv;
      break;
    case KeyEvent::VK_ENTER:
      buf += "Enter"sv;
      break;
    case KeyEvent::VK_ESCAPE:
      buf += "Esc"sv;
      break;
    default:
      buf += '^';
      buf += (char) (key_code + 0x40);
      break;
    }
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
      buf += "F1"sv;
      break;
    case KeyEvent::VK_F2:
      buf += "F2"sv;
      break;
    case KeyEvent::VK_F3:
      buf += "F3"sv;
      break;
    case KeyEvent::VK_F4:
      buf += "F4"sv;
      break;
    case KeyEvent::VK_F5:
      buf += "F5"sv;
      break;
    case KeyEvent::VK_F6:
      buf += "F6"sv;
      break;
    case KeyEvent::VK_F7:
      buf += "F7"sv;
      break;
    case KeyEvent::VK_F8:
      buf += "F8"sv;
      break;
    case KeyEvent::VK_F9:
      buf += "F9"sv;
      break;
    case KeyEvent::VK_F10:
      buf += "F10"sv;
      break;
    case KeyEvent::VK_F11:
      buf += "F11"sv;
      break;
    case KeyEvent::VK_F12:
      buf += "F12"sv;
      break;
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
      buf += "UNDEFINED"sv;
    }
  }

  return buf;
}

struct EventDump {
  std::ostream &os;

  void operator()(const std::monostate&) {
    this->os << "UNDEFINED";
  }

  void operator()(const ActionEvent &event) {
    this->os << "Action '" << event.action_command << "' Performed";
  }

  void operator()(const ItemEvent &e) {

  }

  void operator()(const KeyEvent &event) {
    switch (event.type) {
    case KeyEvent::KEY_PRESSED:
      os << "Key Pressed: ";
      break;

    case KeyEvent::KEY_TYPED:
      os << "Key Typed: ";
      break;
    }

    if (event.modifiers & KeyEvent::ALT_DOWN) {
      os << "Alt+";
    }
    if (event.modifiers & KeyEvent::SHIFT_DOWN) {
      os << "Shift+";
    }
    if (event.modifiers & KeyEvent::CTRL_DOWN) {
      os << "Ctrl+";
    }

    if (event.type == KeyEvent::KEY_TYPED) {
      os << event.get_key_char();
    } else {
      os << to_string(event.get_key_code());
    }
  }

  void operator()(const MouseEvent &event) {
    this->os << "Mouse" << ' ';
    switch (event.button) {
    case MouseEvent::LEFT_BUTTON:
      this->os << "Left Button";
      break;
    case MouseEvent::MIDDLE_BUTTON:
      this->os << "Middle Button";
      break;
    case MouseEvent::RIGHT_BUTTON:
      this->os << "Right Button";
      break;
    case MouseEvent::NO_BUTTON:
      this->os << "No Button";
      break;
    }

    this->os << ' ';

    switch (event.type) {
    case MouseEvent::MOUSE_PRESSED:
      this->os << "Pressed";
      break;
    case MouseEvent::MOUSE_RELEASED:
      this->os << "Released";
      break;

    default:
      break;
    }
    this->os << " at " << event.x << "," << event.y;
  }

  void operator()(const MouseMoveEvent &event) {
    this->os << "Mouse Moved";
    this->os << " at " << event.x << "," << event.y;
  }

  void operator()(const MouseDragEvent &event) {
    this->os << "Mouse Dragged with ";
    switch (event.button) {
    case MouseEvent::LEFT_BUTTON:
      this->os << "Left Button";
      break;
    case MouseEvent::MIDDLE_BUTTON:
      this->os << "Middle Button";
      break;
    case MouseEvent::RIGHT_BUTTON:
      this->os << "Right Button";
      break;
    case MouseEvent::NO_BUTTON:
      this->os << "No Button";
      break;
    }
    this->os << " at " << event.x << "," << event.y;
  }

  void operator()(const MouseClickEvent &event) {
    this->os << "Mouse" << ' ';
    if (event.click_count == 2) {
      this->os << "Double Clicked";
    } else {
      this->os << "Clicked";
    }
    this->os << " at " << event.x << "," << event.y;
  }

  void operator()(const MouseWheelEvent &event) {
    this->os << "Mouse Wheel scrolled by " << event.wheel_rotation;
    this->os << " at " << event.x << "," << event.y;
  }

  void operator()(const MouseHoverEvent &e) {

  }

  void operator()(const InvocationEvent &e) {

  }

  void operator()(const FocusEvent &event) {
    switch (event.type) {
    case FocusEvent::FOCUS_GAINED:
      this->os << "Focus Gained";
      break;
    case FocusEvent::FOCUS_LOST:
      this->os << "Focus Lost";
      break;
    }
  }

  void operator()(const WindowEvent &event) {
    switch (event.type) {
    }
  }
};

std::ostream& operator<<(std::ostream &os, const Event &event) {
  std::visit(EventDump { os }, event);
  return os;
}

}
