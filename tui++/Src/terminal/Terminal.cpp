#include <iostream>

#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalGraphics.h>

using namespace std::string_view_literals;

namespace tui::terminal {

template<typename P, typename ...Params>
static void print_ocs(const P &param, const Params &... params) {
  std::cout << "\x1b]"sv << param;

  [[maybe_unused]] auto add_param = [](const auto &param) {
    std::cout << ';' << param;
  };

  (add_param(params),...);
  // https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences:
  // BEL (0x7) may be used instead as the terminator, but the longer form is preferred.
  std::cout << "\x1b\\"sv;
}

void Terminal::set_option(Option option) {
  struct SetOption {
    void operator()(const DECModeOption &option) {
      std::cout << "\x1b[?" << int(option) << 'h';
    }
    void operator()(const ModifyKeyboardOption &option) {
      std::cout << "\x1b[>0;" << int(option) << 'm';
    }
    void operator()(const ModifyCursorKeysOption &option) {
      std::cout << "\x1b[>1;" << int(option) << 'm';
    }
    void operator()(const ModifyFunctionKeysOption &option) {
      std::cout << "\x1b[>2;" << int(option) << 'm';
    }
    void operator()(const ModifyOtherKeysOption &option) {
      std::cout << "\x1b[>4;" << int(option) << 'm';
    }
  };

  std::visit(SetOption(), option);
  set_options.emplace_back(option);
}

void Terminal::reset_option(Option option) {
  struct ResetOption {
    void operator()(const DECModeOption &option) {
      std::cout << "\x1b[?" << int(option) << 'l';
    }
    void operator()(const ModifyKeyboardOption&) {
      std::cout << "\x1b[>0m";
    }
    void operator()(const ModifyCursorKeysOption&) {
      std::cout << "\x1b[>1m";
    }
    void operator()(const ModifyFunctionKeysOption&) {
      std::cout << "\x1b[>2m";
    }
    void operator()(const ModifyOtherKeysOption&) {
      std::cout << "\x1b[>4m";
    }
  };

  std::visit(ResetOption(), option);

  set_options.erase( //
      std::remove(set_options.begin(), set_options.end(), option), //
      set_options.end());
}

void Terminal::init() {
  set_option(DECModeOption::USE_ALTERNATE_SCREEN_BUFFER);
  reset_option(DECModeOption::LINE_WRAP);
  set_option(DECModeOption::MOUSE_VT200);
  set_option(DECModeOption::MOUSE_ANY_EVENT);
  set_option(DECModeOption::MOUSE_URXVT_EXT_MODE);
  set_option(DECModeOption::MOUSE_SGR_EXT_MODE);

  hide_cursor();

  flush();
}

void Terminal::deinit() {
  reset_option(DECModeOption::MOUSE_SGR_EXT_MODE);
  reset_option(DECModeOption::MOUSE_URXVT_EXT_MODE);
  reset_option(DECModeOption::MOUSE_ANY_EVENT);
  reset_option(DECModeOption::MOUSE_VT200);
  reset_option(DECModeOption::USE_ALTERNATE_SCREEN_BUFFER);

  flush();
}

void Terminal::hide_cursor() {
  reset_option(DECModeOption::CURSOR);
}

void Terminal::show_cursor(Cursor cursor) {
  set_option(DECModeOption::CURSOR);
  std::cout << "\x1b[" << int(cursor) << " q";
}

void Terminal::move_cursor_to(int line, int column) {
  std::cout << "\x1b[" << line << ';' << column << 'H';
}

void Terminal::move_cursor_by(int lines, int columns) {
  if (lines > 0) {
    std::cout << "\x1b[" << lines << 'B';
  } else if (lines < 0) {
    std::cout << "\x1b[" << -lines << 'A';
  }

  if (columns > 0) {
    std::cout << "\x1b[" << columns << 'C';
  } else if (columns < 0) {
    std::cout << "\x1b[" << -columns << 'D';
  }
}

void Terminal::flush() {
  std::cout << std::flush;
}

void Terminal::set_title(const std::string &title) {
  print_ocs('0', title);
  flush();
}

void Terminal::new_resize_event() {
  this->screen.terminal_resized();
}

void Terminal::new_key_event(const Char &c, InputEvent::Modifiers key_modifiers) {
  this->screen.post_system<KeyEvent>(KeyboardFocusManager::get_focused_window(), c, key_modifiers);
}

void Terminal::new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers key_modifiers) {
  this->screen.post_system<KeyEvent>(KeyboardFocusManager::get_focused_window(), KeyEvent::KEY_PRESSED, key_code, key_modifiers);
}

void Terminal::new_mouse_event(MouseEvent::Type type, MouseEvent::Button button, InputEvent::Modifiers key_modifiers, int x, int y) {
  auto motion = false;
  if ((prev_mouse_event.type == type and prev_mouse_event.button == button) or button == MouseEvent::NO_BUTTON) {
    if (prev_mouse_event.x == x and prev_mouse_event.y == y) {
      return;
    } else {
      motion = true;
    }
  }

  modifiers = (modifiers & ~(InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN)) | key_modifiers;

  if (type == MouseEvent::MOUSE_PRESSED) {
    if (button != MouseEvent::NO_BUTTON) {
      modifiers |= to_modifier(button);
    }
  } else if (type == MouseEvent::MOUSE_RELEASED) {
    modifiers &= ~to_modifier(button);
  }

  auto p = Point { x, y };

  auto window = this->screen.get_window_at(p);
  if (window) {
    p = convert_point_from_screen(p, window);
  }

  if (motion) {
    if (type == MouseEvent::MOUSE_PRESSED and button != MouseEvent::NO_BUTTON) {
      this->screen.post_system<MouseDragEvent>(window, button, modifiers, p.x, p.y);
    } else {
      this->screen.post_system<MouseMoveEvent>(window, modifiers, p.x, p.y);
    }
  } else {
    this->screen.post_system<MouseEvent>(window, type, button, modifiers, p.x, p.y, false);

    if (type == MouseEvent::MOUSE_PRESSED) {
      prev_mouse_press_time = Clock::now();
    } else if (type == MouseEvent::MOUSE_RELEASED) {
      if (prev_mouse_event.button == button and (Clock::now() - prev_mouse_press_time) < mouse_click_detection_timeout) {
        auto click_count = 1;
        if ((Clock::now() - prev_mouse_click_time) < mouse_double_click_detection_timeout) {
          click_count = 2;
          prev_mouse_click_time = { };
        } else {
          prev_mouse_click_time = Clock::now();
        }
        this->screen.post_system<MouseClickEvent>(window, button, modifiers, p.x, p.y, click_count, false);
      }
    }
  }

  prev_mouse_event.type = type;
  prev_mouse_event.button = button;
  prev_mouse_event.x = x;
  prev_mouse_event.y = y;
}

void Terminal::new_mouse_wheel_event(int wheel_rotation, InputEvent::Modifiers key_modifiers, int x, int y) {
  auto p = Point { x, y };

  auto window = this->screen.get_window_at(p);
  if (window) {
    p = convert_point_from_screen(p, window);
  }

  this->screen.post_system<MouseWheelEvent>(window, key_modifiers, p.x, p.y, wheel_rotation);
}

std::shared_ptr<TerminalGraphics> Terminal::get_graphics() {
  return std::make_shared<TerminalGraphics>(this->screen);
}

}
