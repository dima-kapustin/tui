#include <tui++/Event.h>
#include <tui++/Component.h>

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

std::ostream& operator<<(std::ostream &os, const ActionEvent &event) {
  return os << "Action '" << event.action_command << "' PERFORMED";
}

std::ostream& operator<<(std::ostream &os, const ItemEvent &event) {
  return os;
}

std::ostream& operator<<(std::ostream &os, const KeyEvent &event) {
  switch (event.id) {
  case KeyEvent::KEY_PRESSED:
    os << "Key PRESSED: ";
    break;

  case KeyEvent::KEY_TYPED:
    os << "Key TYPED: ";
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

  if (event.id == KeyEvent::KEY_TYPED) {
    os << event.get_key_char();
  } else {
    os << to_string(event.get_key_code());
  }
  return os;
}

std::ostream& operator<<(std::ostream &os, const MousePressEvent &event) {
  os << "Mouse" << ' ';
  switch (event.button) {
  case MousePressEvent::LEFT_BUTTON:
    os << "Left Button";
    break;
  case MousePressEvent::MIDDLE_BUTTON:
    os << "Middle Button";
    break;
  case MousePressEvent::RIGHT_BUTTON:
    os << "Right Button";
    break;
  case MousePressEvent::NO_BUTTON:
    os << "No Button";
    break;
  }

  os << ' ';

  switch (event.id) {
  case MousePressEvent::MOUSE_PRESSED:
    os << "PRESSED";
    break;
  case MousePressEvent::MOUSE_RELEASED:
    os << "RELEASED";
    break;

  default:
    break;
  }
  os << " at " << event.x << "," << event.y;
  return os;
}

std::ostream& operator<<(std::ostream &os, const MouseMoveEvent &event) {
  os << "Mouse MOVED";
  os << " at " << event.x << "," << event.y;
  return os;
}

std::ostream& operator<<(std::ostream &os, const MouseDragEvent &event) {
  os << "Mouse DRAGGED with ";
  switch (event.button) {
  case MousePressEvent::LEFT_BUTTON:
    os << "Left Button";
    break;
  case MousePressEvent::MIDDLE_BUTTON:
    os << "Middle Button";
    break;
  case MousePressEvent::RIGHT_BUTTON:
    os << "Right Button";
    break;
  case MousePressEvent::NO_BUTTON:
    os << "No Button";
    break;
  }
  os << " at " << event.x << "," << event.y;
  return os;
}

std::ostream& operator<<(std::ostream &os, const MouseClickEvent &event) {
  os << "Mouse" << ' ';
  if (event.click_count == 2) {
    os << "DOUBLE CLICKED";
  } else {
    os << "CLICKED";
  }
  os << " at " << event.x << "," << event.y;
  return os;
}

std::ostream& operator<<(std::ostream &os, const MouseWheelEvent &event) {
  os << "Mouse WHEEL scrolled by " << event.wheel_rotation;
  os << " at " << event.x << "," << event.y;
  return os;
}

std::ostream& operator<<(std::ostream &os, const MouseOverEvent &event) {
  switch (event.id) {
  case MouseOverEvent::MOUSE_ENTERED:
    os << "Mouse ENTERED " << event.component()->get_name();
    break;
  case MouseOverEvent::MOUSE_EXITED:
    os << "Mouse EXITED " << event.component()->get_name();
    break;
  }
  return os;
}

std::ostream& operator<<(std::ostream &os, const InvocationEvent &event) {
  return os;
}

std::ostream& operator<<(std::ostream &os, const FocusEvent &event) {
  switch (event.id) {
  case FocusEvent::FOCUS_GAINED:
    os << "Focus GAINED";
    break;
  case FocusEvent::FOCUS_LOST:
    os << "Focus LOST";
    break;
  }
  return os;
}

std::ostream& operator<<(std::ostream &os, const WindowEvent &event) {
  switch (event.id) {
  case WindowEvent::WINDOW_OPENED:
    os << "Window OPENED";
    break;
  case WindowEvent::WINDOW_CLOSING:
    os << "Window CLOSING";
    break;
  case WindowEvent::WINDOW_CLOSED:
    os << "Window CLOSED";
    break;
  case WindowEvent::WINDOW_ACTIVATED:
    os << "Window ACTIVATED";
    break;
  case WindowEvent::WINDOW_DEACTIVATED:
    os << "Window DEACTIVATED";
    break;
  case WindowEvent::WINDOW_GAINED_FOCUS:
    os << "Window GAINED FOCUS";
    break;
  case WindowEvent::WINDOW_LOST_FOCUS:
    os << "Window LOST FOCUS";
    break;
  case WindowEvent::WINDOW_ICONIFIED:
    os << "Window ICONIFIED";
    break;
  case WindowEvent::WINDOW_DEICONIFIED:
    os << "Window DEICONIFIED";
    break;
  case WindowEvent::WINDOW_STATE_CHANGED:
    os << "Window STATE CHANGED";
    break;
  }
  return os;
}

std::ostream& operator<<(std::ostream &os, const ComponentEvent &event) {
  return os;
}

std::ostream& operator<<(std::ostream &os, const ContainerEvent &event) {
  return os;
}

std::ostream& operator<<(std::ostream &os, const HierarchyEvent &event) {
  return os;
}

std::ostream& operator<<(std::ostream &os, const HierarchyBoundsEvent &event) {
  return os;
}

std::ostream& operator<<(std::ostream &os, const Event &event) {
  switch (EventType(event.id.type)) {
  case EventType::ACTION:
    os << static_cast<const ActionEvent&>(event);
    break;
  case EventType::FOCUS:
    os << static_cast<const FocusEvent&>(event);
    break;
  case EventType::COMPONENT:
    os << static_cast<const ComponentEvent&>(event);
    break;
  case EventType::CONTAINER:
    os << static_cast<const ContainerEvent&>(event);
    break;
  case EventType::INVOCATION:
    os << static_cast<const InvocationEvent&>(event);
    break;
  case EventType::ITEM:
    os << static_cast<const ItemEvent&>(event);
    break;
  case EventType::HIERARCHY:
    os << static_cast<const HierarchyEvent&>(event);
    break;
  case EventType::HIERARCHY_BOUNDS:
    os << static_cast<const HierarchyBoundsEvent&>(event);
    break;
  case EventType::KEY:
    os << static_cast<const KeyEvent&>(event);
    break;
  case EventType::MOUSE_PRESS:
    os << static_cast<const MousePressEvent&>(event);
    break;
  case EventType::MOUSE_MOVE:
    os << static_cast<const MouseMoveEvent&>(event);
    break;
  case EventType::MOUSE_DRAG:
    os << static_cast<const MouseDragEvent&>(event);
    break;
  case EventType::MOUSE_OVER:
    os << static_cast<const MouseOverEvent&>(event);
    break;
  case EventType::MOUSE_CLICK:
    os << static_cast<const MouseClickEvent&>(event);
    break;
  case EventType::MOUSE_WHEEL:
    os << static_cast<const MouseWheelEvent&>(event);
    break;
  case EventType::WINDOW:
    os << static_cast<const WindowEvent&>(event);
    break;
  }
  return os;
}

}
