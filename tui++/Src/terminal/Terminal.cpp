#include <iostream>

#include <tui++/Component.h>

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

void Terminal::new_key_event(const Char &c, InputEvent::Modifiers modifiers) {
  this->screen.post(std::make_unique<Event>(nullptr, c, modifiers));
}

void Terminal::new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) {
  this->screen.post(std::make_unique<Event>(nullptr, key_code, modifiers));
}

void Terminal::new_mouse_event(MouseEvent::Type type, MouseEvent::Button button, InputEvent::Modifiers modifiers, int x, int y) {
  auto wheel_rotation = 0;
  auto adjusted_type = type;
  if (type != MouseEvent::MOUSE_WHEEL) {
    if ((prev_mouse_event.type == type and prev_mouse_event.button == button) or button == MouseEvent::NO_BUTTON) {
      if (prev_mouse_event.x == x and prev_mouse_event.y == y) {
        return;
      } else {
        if (type == MouseEvent::MOUSE_PRESSED and button != MouseEvent::NO_BUTTON) {
          adjusted_type = MouseEvent::MOUSE_DRAGGED;
        } else {
          adjusted_type = MouseEvent::MOUSE_MOVED;
        }
      }
    }
  } else {
    wheel_rotation = button == 0 ? -1 : 1;
  }

  auto p = Point { x, y };

  auto component = this->screen.get_component_at(p);
  if (component) {
    p = convert_point_from_screen(p, component);
  }

  this->screen.post(std::make_unique<Event>(component, adjusted_type, button, modifiers, p.x, p.y, wheel_rotation, false));

  if (adjusted_type == MouseEvent::MOUSE_PRESSED) {
    prev_mouse_press_time = Clock::now();
  } else if (adjusted_type == MouseEvent::MOUSE_RELEASED) {
    if (prev_mouse_event.button == button and (Clock::now() - prev_mouse_press_time) < mouse_click_detection_timeout) {
      if ((Clock::now() - prev_mouse_click_time) < mouse_double_click_detection_timeout) {
        this->screen.post(std::make_unique<Event>(component, MouseEvent::MOUSE_CLICKED, button, modifiers, p.x, p.y, 2, false));
        prev_mouse_click_time = { };
      } else {
        this->screen.post(std::make_unique<Event>(component, MouseEvent::MOUSE_CLICKED, button, modifiers, p.x, p.y, 1, false));
        prev_mouse_click_time = Clock::now();
      }
    }
  }

  prev_mouse_event.type = type;
  prev_mouse_event.button = button;
  prev_mouse_event.modifiers = modifiers;
  prev_mouse_event.x = x;
  prev_mouse_event.y = y;
}

std::shared_ptr<TerminalGraphics> Terminal::get_graphics() {
  return std::make_shared<TerminalGraphics>(this->screen);
}

}
