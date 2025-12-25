#include <iostream>

#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalGraphics.h>

using namespace std::string_view_literals;

static std::atomic<unsigned> terminal_ref_counter = 0;
static std::byte terminal_buf[sizeof(tui::terminal::Terminal)];
static std::byte terminal_screen_buf[sizeof(tui::terminal::TerminalScreen)];

namespace tui {
terminal::Terminal &this_terminal = reinterpret_cast<terminal::Terminal&>(terminal_buf);
terminal::TerminalScreen &this_terminal_screen = reinterpret_cast<terminal::TerminalScreen&>(terminal_screen_buf);
}

namespace tui::terminal {

Terminal& Terminal::get_singleton() {
  if (terminal_ref_counter++ == 0) {
    ::new (&this_terminal) Terminal();
    ::new (&this_terminal_screen) TerminalScreen();
  }
  return this_terminal;
}

Terminal::Singleton::Singleton() {
  Terminal::get_singleton();
}

Terminal::Singleton::~Singleton() {
  if (--terminal_ref_counter == 0) {
    this_terminal_screen.~TerminalScreen();
    this_terminal.~Terminal();
  }
}

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

void Terminal::hide_cursor() {
  reset_option(DECModeOption::CURSOR);
}

void Terminal::show_cursor(Cursor cursor) {
  set_option(DECModeOption::CURSOR);
  std::cout << "\x1b["sv << int(cursor) << " q"sv;
}

void Terminal::set_option(Option option) {
  struct SetOption {
    void operator()(const DECModeOption &option) {
      std::cout << "\x1b[?"sv << int(option) << 'h';
    }
    void operator()(const ModifyKeyboardOption &option) {
      std::cout << "\x1b[>0;"sv << int(option) << 'm';
    }
    void operator()(const ModifyCursorKeysOption &option) {
      std::cout << "\x1b[>1;"sv << int(option) << 'm';
    }
    void operator()(const ModifyFunctionKeysOption &option) {
      std::cout << "\x1b[>2;"sv << int(option) << 'm';
    }
    void operator()(const ModifyOtherKeysOption &option) {
      std::cout << "\x1b[>4;"sv << int(option) << 'm';
    }
  };

  std::visit(SetOption { }, option);
  set_options.emplace_back(option);
}

void Terminal::reset_option(Option option) {
  struct ResetOption {
    void operator()(const DECModeOption &option) {
      std::cout << "\x1b[?"sv << int(option) << 'l';
    }
    void operator()(const ModifyKeyboardOption&) {
      std::cout << "\x1b[>0m"sv;
    }
    void operator()(const ModifyCursorKeysOption&) {
      std::cout << "\x1b[>1m"sv;
    }
    void operator()(const ModifyFunctionKeysOption&) {
      std::cout << "\x1b[>2m"sv;
    }
    void operator()(const ModifyOtherKeysOption&) {
      std::cout << "\x1b[>4m"sv;
    }
  };

  std::visit(ResetOption { }, option);

  set_options.erase( //
      std::remove(set_options.begin(), set_options.end(), option), //
      set_options.end());
}

void Terminal::init() {
  std::ios_base::sync_with_stdio(false);

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

void Terminal::set_title(const std::string &title) {
  print_ocs('0', title);
  flush();
}

void Terminal::new_resize_event() {
  this_terminal_screen.terminal_resized();
}

void Terminal::new_key_event(const Char &c, InputEvent::Modifiers key_modifiers) {
  this_terminal_screen.post_system<KeyEvent>(KeyboardFocusManager::single->get_focused_window(), c, key_modifiers);
}

void Terminal::new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers key_modifiers) {
  this_terminal_screen.post_system<KeyEvent>(KeyboardFocusManager::single->get_focused_window(), KeyEvent::KEY_PRESSED, key_code, key_modifiers);
}

void Terminal::new_mouse_event(MousePressEvent::Type type, MousePressEvent::Button button, InputEvent::Modifiers key_modifiers, int x, int y) {
  auto motion = false;
  if ((prev_mouse_event.type == type and prev_mouse_event.button == button) or button == MousePressEvent::NO_BUTTON) {
    if (prev_mouse_event.x == x and prev_mouse_event.y == y) {
      return;
    } else {
      motion = true;
    }
  }

  modifiers = (modifiers & ~(InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN)) | key_modifiers;

  if (type == MousePressEvent::MOUSE_PRESSED) {
    if (button != MousePressEvent::NO_BUTTON) {
      modifiers |= to_modifier(button);
    }
  } else if (type == MousePressEvent::MOUSE_RELEASED) {
    modifiers &= ~to_modifier(button);
  }

  auto p = Point { x, y };

  auto window = this_terminal_screen.get_window_at(p);
  if (window) {
    p = convert_point_from_screen(p, window);
  }

  if (motion) {
    if (type == MousePressEvent::MOUSE_PRESSED and button != MousePressEvent::NO_BUTTON) {
      this_terminal_screen.post_system<MouseDragEvent>(window, button, modifiers, p.x, p.y);
    } else {
      this_terminal_screen.post_system<MouseMoveEvent>(window, modifiers, p.x, p.y);
    }
  } else {
    this_terminal_screen.post_system<MousePressEvent>(window, type, button, modifiers, p.x, p.y, false);

    if (type == MousePressEvent::MOUSE_PRESSED) {
      prev_mouse_press_time = Clock::now();
    } else if (type == MousePressEvent::MOUSE_RELEASED) {
      if (prev_mouse_event.button == button and (Clock::now() - prev_mouse_press_time) < mouse_click_detection_timeout) {
        auto click_count = 1;
        if ((Clock::now() - prev_mouse_click_time) < mouse_double_click_detection_timeout) {
          click_count = 2;
          prev_mouse_click_time = { };
        } else {
          prev_mouse_click_time = Clock::now();
        }
        this_terminal_screen.post_system<MouseClickEvent>(window, button, modifiers, p.x, p.y, click_count, false);
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

  auto window = this_terminal_screen.get_window_at(p);
  if (window) {
    p = convert_point_from_screen(p, window);
  }

  this_terminal_screen.post_system<MouseWheelEvent>(window, key_modifiers, p.x, p.y, wheel_rotation);
}

TerminalScreen& Terminal::get_screen() {
  return this_terminal_screen;
}

std::shared_ptr<TerminalGraphics> Terminal::get_graphics() {
  return std::make_shared<TerminalGraphics>(this_terminal_screen);
}

Terminal& Terminal::write(const char *data, size_t size) {
  std::cout.write(data, size);
  return *this;
}

void Terminal::flush() {
  std::cout << std::flush;
}

Terminal& operator<<(Terminal &term, std::string_view const &value) {
  return term.write(value.data(), value.size());
}

Terminal& operator<<(Terminal &term, std::string const &value) {
  return term.write(value.data(), value.size());
}

Terminal& operator<<(Terminal &term, char value) {
  return term.write(&value, 1);
}

Terminal& operator<<(Terminal &term, unsigned value) {
  char buf[16];
  auto &&result = std::to_chars(buf, buf + std::size(buf), value);
  return term.write(buf, result.ptr - buf);
}

Terminal& operator<<(Terminal &term, signed value) {
  char buf[16];
  auto &&result = std::to_chars(buf, buf + std::size(buf), value);
  return term.write(buf, result.ptr - buf);
}

}
