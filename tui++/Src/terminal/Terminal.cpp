#include <iostream>

#include <cstdio>
#include <cstdlib>

#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>
#include <tui++/terminal/text/TextGraphics.h>
#include <tui++/terminal/sixel/SixelScreen.h>

using namespace std::string_view_literals;

static std::atomic<unsigned> terminal_ref_counter = 0;
static std::byte terminal_buf[sizeof(tui::Terminal)];
static std::byte screen_buf[std::max(sizeof(tui::TextScreen), sizeof(tui::SixelScreen))];

namespace tui {
Terminal &terminal = reinterpret_cast<Terminal&>(terminal_buf);
Screen &screen = reinterpret_cast<Screen&>(screen_buf);
}

namespace tui {

Terminal& Terminal::get_singleton() {
  if (terminal_ref_counter++ == 0) {
    ::new (&terminal) Terminal();
    ::new (&screen) TextScreen();
  }
  return terminal;
}

Terminal::Singleton::Singleton() {
  Terminal::get_singleton();
}

Terminal::Singleton::~Singleton() {
  if (--terminal_ref_counter == 0) {
    screen.~Screen();
    terminal.~Terminal();
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

void Terminal::InputParser::new_mouse_event(bool pressed) {
  auto button = this->csi_params[0] & 3;
  auto key_modifiers = this->csi_params[0] & 4 ? InputEvent::SHIFT_DOWN : InputEvent::NO_MODIFIERS;
  key_modifiers |= this->csi_params[0] & 8 ? InputEvent::META_DOWN : InputEvent::NO_MODIFIERS;
  key_modifiers |= this->csi_params[0] & 16 ? InputEvent::CTRL_DOWN : InputEvent::NO_MODIFIERS;
  auto x = this->csi_params[1] - 1;
  auto y = this->csi_params[2] - 1;
  if (this->csi_params[0] & 64) {
    this->terminal.new_mouse_wheel_event(button == 0 ? -1 : 1, key_modifiers, x, y);
  } else {
    auto type = pressed ? MousePressEvent::MOUSE_PRESSED : MousePressEvent::MOUSE_RELEASED;
    this->terminal.new_mouse_event(type, MousePressEvent::Button(button), key_modifiers, x, y);
  }
}

void Terminal::hide_cursor() {
  reset_option(DECModeOption::CURSOR);
}

void Terminal::show_cursor(std::optional<Cursor> const &cursor) {
  set_option(DECModeOption::CURSOR);
  if (cursor) {
    std::cout << "\x1b["sv << int(cursor->type()) << " q"sv;
  }
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

  // Keep the terminal's default sixel cursor behaviour: the cursor advances
  // past each image's bottom-right corner (the mode set variant makes some
  // terminals stop honoring the cursor position for later images). The
  // graphic screen keeps images short of the bottom-right corner instead.
  reset_option(DECModeOption::SIXEL_DISPLAY_MODE);

  hide_cursor();

  flush();
}

void Terminal::deinit() {
  reset_option(DECModeOption::MOUSE_SGR_EXT_MODE);
  reset_option(DECModeOption::MOUSE_URXVT_EXT_MODE);
  reset_option(DECModeOption::MOUSE_ANY_EVENT);
  reset_option(DECModeOption::MOUSE_VT200);
  reset_option(DECModeOption::USE_ALTERNATE_SCREEN_BUFFER);
  set_option(DECModeOption::SIXEL_DISPLAY_MODE);

  // The cursor and line wrap are changed outside the option list (init()
  // hides the cursor and resets line wrap), so restore them here too. This
  // matches RESTORE_SEQUENCE, which the signal handlers emit on abnormal
  // exits.
  std::cout << "\x1b[?25h\x1b[?7h"sv;

  flush();
}

void Terminal::set_title(const std::string &title) {
  print_ocs('0', title);
  flush();
}

void Terminal::new_resize_event() {
  screen.resized();
}

void Terminal::new_key_event(const Char &c, InputEvent::Modifiers key_modifiers) {
  screen.post_system<KeyEvent>(KeyboardFocusManager::single->get_focused_window(), c, key_modifiers);
}

void Terminal::new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers key_modifiers) {
  screen.post_system<KeyEvent>(KeyboardFocusManager::single->get_focused_window(), KeyEvent::KEY_PRESSED, key_code, key_modifiers);
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
      modifiers |= to_modifiers(button);
    }
  } else if (type == MousePressEvent::MOUSE_RELEASED) {
    modifiers &= ~to_modifiers(button);
  }

  auto p = screen.convert_mouse_point(x, y);

  auto window = screen.get_window_at(p);
  if (window) {
    p = convert_point_from_screen(p, window);
  }

  if (motion) {
    if (type == MousePressEvent::MOUSE_PRESSED and button != MousePressEvent::NO_BUTTON) {
      screen.post_system<MouseDragEvent>(window, button, modifiers, p.x, p.y);
    } else {
      screen.post_system<MouseMoveEvent>(window, modifiers, p.x, p.y);
    }
  } else {
    screen.post_system<MousePressEvent>(window, type, button, modifiers, p.x, p.y, false);

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
        screen.post_system<MouseClickEvent>(window, button, modifiers, p.x, p.y, click_count, false);
      }
    }
  }

  prev_mouse_event.type = type;
  prev_mouse_event.button = button;
  prev_mouse_event.x = x;
  prev_mouse_event.y = y;
}

void Terminal::new_mouse_wheel_event(int wheel_rotation, InputEvent::Modifiers key_modifiers, int x, int y) {
  auto p = screen.convert_mouse_point(x, y);

  auto window = screen.get_window_at(p);
  if (window) {
    p = convert_point_from_screen(p, window);
  }

  screen.post_system<MouseWheelEvent>(window, key_modifiers, p.x, p.y, wheel_rotation);
}

Screen& Terminal::get_screen() {
  return screen;
}

std::optional<Dimension> Terminal::query_cell_size_from_terminal() {
  // Ask for the cell size directly and, as a fallback, for the text area in
  // pixels and in characters (the cell size is then pixels / characters).
  std::cout << "\x1b[16t\x1b[14t\x1b[18t" << std::flush;

  auto deadline = Clock::now() + std::chrono::milliseconds(150);
  std::string reply;
  reply.reserve(64);
  InputReader reader { *this };
  while (Clock::now() < deadline) {
    auto ms = std::max(int64_t(1), std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count());
    auto c = reader.get(std::chrono::milliseconds(ms));
    if (c == 0) {
      break; // no more input before the deadline
    }
    reply += c;
    while (auto more = reader.consume()) {
      reply += more;
    }

    // CSI 6 ; height ; width t -- the cell size directly.
    if (auto pos = reply.find("\x1b[6;"); pos != std::string::npos) {
      auto height = 0, width = 0;
      if (std::sscanf(reply.c_str() + pos, "\x1b[6;%d;%dt", &height, &width) == 2 and height > 0 and width > 0) {
        return Dimension { width, height };
      }
    }
  }

  // Fallback: text area in pixels (CSI 4 ; h ; w t) divided by the text area
  // in characters (CSI 8 ; r ; c t).
  auto px_h = 0, px_w = 0, ch_h = 0, ch_w = 0;
  if (auto pos = reply.find("\x1b[4;"); pos != std::string::npos) {
    std::sscanf(reply.c_str() + pos, "\x1b[4;%d;%dt", &px_h, &px_w);
  }
  if (auto pos = reply.find("\x1b[8;"); pos != std::string::npos) {
    std::sscanf(reply.c_str() + pos, "\x1b[8;%d;%dt", &ch_h, &ch_w);
  }
  if (px_h > 0 and px_w > 0 and ch_h > 0 and ch_w > 0) {
    return Dimension { std::max(1, px_w / ch_w), std::max(1, px_h / ch_h) };
  }

  return { };
}

std::optional<bool> Terminal::query_graphics_support() {
  // Primary DA reports what the terminal is; xterm appends Ps = 4 to it when
  // sixel graphics are enabled (a real VT340 reports 62). Secondary DA
  // reports xterm's emulation level: Pp = 2/18/19/32 are the graphics
  // emulations (VT240/VT330/VT340/VT382), and Pp = 0/1/24/41/61/64/65 with
  // the xterm firmware signature (Pc = 0, patch level Pv >= 95) are xterm
  // emulations without graphics.
  std::cout << "\x1b[c\x1b[>c" << std::flush;

  auto deadline = Clock::now() + std::chrono::milliseconds(250);
  std::string reply;
  reply.reserve(64);
  InputReader reader { *this };
  while (Clock::now() < deadline) {
    auto ms = std::max(int64_t(1), std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count());
    auto c = reader.get(std::chrono::milliseconds(ms));
    if (c == 0) {
      break; // no more input before the deadline
    }
    reply += c;
    while (auto more = reader.consume()) {
      reply += more;
    }

    // Primary DA: `CSI ? Ps ; ... c`.
    if (auto pos = reply.find("\x1b[?"); pos != std::string::npos) {
      if (auto end = reply.find('c', pos); end != std::string::npos) {
        auto val = 0;
        for (auto i = pos + 3; i < end; ++i) {
          if (reply[std::size_t(i)] == ';') {
            if (val == 4 or val == 62) {
              return true;
            }
            val = 0;
          } else if (reply[std::size_t(i)] >= '0' and reply[std::size_t(i)] <= '9') {
            val = val * 10 + (reply[std::size_t(i)] - '0');
          }
        }
        if (val == 4 or val == 62) {
          return true;
        }
      }
    }

    // Secondary DA: `CSI > Pp ; Pv ; Pc c`.
    if (auto pos = reply.find("\x1b[>"); pos != std::string::npos) {
      if (auto end = reply.find('c', pos); end != std::string::npos) {
        auto pp = 0, pv = 0, pc = -1;
        if (std::sscanf(reply.c_str() + pos + 3, "%d;%d;%d", &pp, &pv, &pc) == 3) {
          if (pp == 2 or pp == 18 or pp == 19 or pp == 32) {
            return true; // a graphics emulation
          }
          if (pc == 0 and pv >= 95 and (pp == 0 or pp == 1 or pp == 24 or pp == 41 or pp == 61 or pp == 64 or pp == 65)) {
            return false; // an xterm emulation without graphics
          }
        }
      }
    }
  }

  return { };
}

std::optional<Dimension> Terminal::query_graphics_geometry() {
  // XTSMGRAPHICS `CSI ? 2 ; 4 S`: item 2 (sixel geometry), action 4 (read
  // the maximum allowed value). xterm replies `CSI ? 2 ; 0 ; W ; H S` with
  // the maxGraphicSize dimensions; terminals without the control (Windows
  // Terminal) answer nothing.
  std::cout << "\x1b[?2;4S" << std::flush;

  auto deadline = Clock::now() + std::chrono::milliseconds(250);
  std::string reply;
  reply.reserve(64);
  InputReader reader { *this };
  while (Clock::now() < deadline) {
    auto ms = std::max(int64_t(1), std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count());
    auto c = reader.get(std::chrono::milliseconds(ms));
    if (c == 0) {
      break;
    }
    reply += c;
    while (auto more = reader.consume()) {
      reply += more;
    }

    if (auto pos = reply.find("\x1b[?2;"); pos != std::string::npos) {
      // The reply is `CSI ? 2 ; Ps ; W ; H S`; parse from the sequence start
      // so the offset into the parameters cannot drift (the prefix is five
      // characters: ESC [ ? 2 ;).
      auto status = 0, width = 0, height = 0;
      if (std::sscanf(reply.c_str() + pos, "\x1b[?2;%d;%d;%dS", &status, &width, &height) == 3 and width > 0 and height > 0) {
        return Dimension { width, height };
      }
    }
  }

  return { };
}

void Terminal::set_type(std::string_view type) {
  if (this->type != type) {
    if (type == "sixel" and not std::getenv("TUI_FORCE_SIXEL")) {
      if (auto support = query_graphics_support(); support.has_value() and not *support) {
        // The terminal identified itself as one without sixel graphics;
        // drawing would produce a blank screen. Leave the alternate screen
        // buffer before printing so the explanation stays visible.
        std::cout << "\x1b[?1049l"sv;
        std::cout << "tui++: sixel graphics are not enabled in this terminal.\n"
                     "  xterm: start it with sixel enabled, e.g.   xterm -ti 340\n"
                     "  (requires an xterm built with sixel support)\n"
                     "  To skip this check, run with TUI_FORCE_SIXEL=1\n" << std::flush;
        std::exit(1);
      }
    }

    screen.~Screen();

    if (type == "sixel") {
      ::new (&screen) SixelScreen();
    } else {
      ::new (&screen) TextScreen();
    }

    this->type = type;
  }
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
