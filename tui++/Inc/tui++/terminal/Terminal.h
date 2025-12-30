#pragma once

#include <chrono>
#include <variant>
#include <iostream>
#include <functional>

#include <tui++/Event.h>
#include <tui++/Cursor.h>
#include <tui++/Dimension.h>

#include <tui++/terminal/TerminalScreen.h>

namespace tui {

class Terminal;
extern Terminal &terminal;

class TerminalImpl;
class TerminalScreen;
class TerminalGraphics;

class Terminal {
  enum class DECModeOption {
    LINE_WRAP = 7,
    CURSOR = 25,

    MOUSE_X10 = 9,
    MOUSE_VT200 = 1000,
    MOUSE_VT200_HIGHLIGHT = 1001,

    MOUSE_BTN_EVENT = 1002,
    MOUSE_ANY_EVENT = 1003,

    MOUSE_EXT_MODE = 1005,
    MOUSE_SGR_EXT_MODE = 1006,
    MOUSE_URXVT_EXT_MODE = 1015,
    MOUSE_PIXEL_POSITION_MODE = 1016,

    // The alternate buffer is exactly the dimensions of the window, without any scrollback region.
    USE_ALTERNATE_SCREEN_BUFFER = 1049
  };

  // Normally xterm makes a special case regarding modifiers (shift, control, etc.)
  // to handle special keyboard layouts (legacy and vt220).
  // This is done to provide compatible keyboards for DEC VT220 and related terminals
  // that implement user-defined keys (UDK).
  //
  // The bits of the resource value selectively enable modification of the given
  // category when these keyboards are selected. The default is "0".
  enum class ModifyKeyboardOption {
    CONTROL_ONLY = 0,
    NUMERIC_KEYPAD = 1,
    EDITING_KEYPAD = 2,
    FUNCTION_KEYS = 4,
    OTHER_SPECIAL_KEYS = 8,

    DEFAULT = CONTROL_ONLY
  };

  // Tells how to handle the special case where Control-, Shift-, Alt- or Meta-modifiers
  // are used to add a parameter to the escape sequence returned by a cursor-key.
  // The default is "2".
  enum class ModifyCursorKeysOption {
    // Permit the user to use shift- and control-modifiers to construct function-key
    // strings using the normal encoding scheme.
    DISABLED = -1,
    // Use the old/obsolete behavior, i.e., the modifier is the first parameter.
    MODIFIER_FIRST = 0,
    // Prefixe modified sequences with CSI
    PREFIX_WITH_CSI = 1,
    // Force the modifier to be the second parameter if it would otherwise be the first.
    MODIFIER_SECOND = 2,
    // Mark the sequence with a ">" to hint that it is private.
    MARK_PRIVATE = 3
  };

  // Tells how to handle the special case where Control-, Shift-, Alt- or Meta-modifiers
  // are used to add a parameter to the escape sequence returned by a (numbered) function-key.
  // The default is "2".
  enum class ModifyFunctionKeysOption {
    // Permit the user to use shift- and control-modifiers to construct function-key
    // strings using the normal encoding scheme.
    DISABLED = -1,
    // Use the old/obsolete behavior, i.e., the modifier is the first parameter.
    MODIFIER_FIRST = 0,
    // Prefixe modified sequences with CSI
    PREFIX_WITH_CSI = 1,
    // Force the modifier to be the second parameter if it would otherwise be the first.
    MODIFIER_SECOND = 2,
    // Mark the sequence with a ">" to hint that it is private.
    MARK_PRIVATE = 3
  };

  enum class ModifyOtherKeysOption {

  };

  using Option = std::variant<DECModeOption, ModifyKeyboardOption, ModifyCursorKeysOption, ModifyFunctionKeysOption, ModifyOtherKeysOption>;

  class InputBuffer {
  protected:
    constexpr static size_t BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    size_t read_pos = 0, write_pos = 0;

  public:
    void put(char c) {
      this->buffer[this->write_pos] = c;
      this->write_pos = (this->write_pos + 1) % BUFFER_SIZE;
    }

    auto get_available() const {
      return (this->write_pos - this->read_pos) % BUFFER_SIZE;
    }
  };

  class InputReader: public InputBuffer {
    Terminal &terminal;

  private:
    bool read_terminal_input(const std::chrono::milliseconds &timeout) {
      return this->terminal.read_input(timeout, *this);
    }

  public:
    InputReader(Terminal &terminal) :
        terminal(terminal) {
    }

    char get() {
      return get(std::chrono::milliseconds::max());
    }

    char get(const std::chrono::milliseconds &timeout) {
      if (this->read_pos == this->write_pos) {
        if (not read_terminal_input(timeout)) {
          return 0;
        }
      }
      return this->buffer[this->read_pos];
    }

    char consume() {
      if (this->read_pos == this->write_pos) {
        if (not read_terminal_input(std::chrono::milliseconds::zero())) {
          return 0;
        }
      }
      auto pos = this->read_pos;
      this->read_pos = (this->read_pos + 1) % BUFFER_SIZE;
      return this->buffer[pos];
    }
  };

  class InputParser {
    Terminal &terminal;
    InputReader reader;
    std::vector<unsigned> csi_params;
    bool csi_altered = false;

  private:
    void parse_esc();

    void parse_ss3();
    void parse_dcs();
    void parse_csi();
    void parse_csi_params();
    void parse_csi_selector();
    void parse_osc();
    void parse_utf8(char first_byte);

  private:
    void new_key_event(const Char &c, InputEvent::Modifiers key_modifiers = InputEvent::NO_MODIFIERS) {
      this->terminal.new_key_event(c, key_modifiers);
    }

    void new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers key_modifiers = InputEvent::NO_MODIFIERS) {
      this->terminal.new_key_event(key_code, key_modifiers);
    }

    void new_mouse_event(bool pressed) {
      auto button = this->csi_params[0] & 3;
      InputEvent::Modifiers key_modifiers = this->csi_params[0] & 4 ? InputEvent::SHIFT_DOWN : InputEvent::NO_MODIFIERS;
      key_modifiers |= this->csi_params[0] & 8 ? InputEvent::META_DOWN : InputEvent::NO_MODIFIERS;
      key_modifiers |= this->csi_params[0] & 16 ? InputEvent::CTRL_DOWN : InputEvent::NO_MODIFIERS;
      int x = this->csi_params[1];
      int y = this->csi_params[2];
      if (this->csi_params[0] & 64) {
        this->terminal.new_mouse_wheel_event(button == 0 ? -1 : 1, key_modifiers, x, y);
      } else {
        auto type = pressed ? MousePressEvent::MOUSE_PRESSED : MousePressEvent::MOUSE_RELEASED;
        this->terminal.new_mouse_event(type, MousePressEvent::Button(button), key_modifiers, x, y);
      }
    }

    char get() {
      return this->reader.get();
    }

    char consume() {
      return this->reader.consume();
    }

    char consume(const std::chrono::milliseconds &timeout) {
      if (auto c = this->reader.get(timeout)) {
        this->reader.consume();
        return c;
      }
      return 0;
    }

  public:
    InputParser(Terminal &terminal) :
        terminal(terminal), reader(terminal) {
    }

    void parse_event();
  };

  using Clock = std::chrono::steady_clock;

private:
  bool quit;

  std::vector<Option> set_options;

  struct {
    MousePressEvent::Type type;
    MousePressEvent::Button button = MousePressEvent::NO_BUTTON;
    int x, y;
  } prev_mouse_event;
  InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS;
  Clock::time_point prev_mouse_press_time;
  Clock::time_point prev_mouse_click_time;

  std::chrono::milliseconds read_input_timeout { 20 };
  std::chrono::milliseconds mouse_click_detection_timeout { 400 };
  std::chrono::milliseconds mouse_double_click_detection_timeout { 300 };

  std::unique_ptr<TerminalImpl> impl;

  InputParser input_parser { *this };

private:
  void set_option(Option option);
  void reset_option(Option option);

  void init();
  void deinit();

  void new_resize_event();
  void new_key_event(const Char &c, InputEvent::Modifiers key_modifiers);
  void new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers key_modifiers);
  void new_mouse_event(MousePressEvent::Type type, MousePressEvent::Button button, InputEvent::Modifiers key_modifiers, int x, int y);
  void new_mouse_wheel_event(int wheel_rotation, InputEvent::Modifiers key_modifiers, int x, int y);
//  void new_mouse_move_event(InputEvent::Modifiers modifiers, int x, int y);
//  void new_mouse_drag_event(MousePressEvent::Button button, InputEvent::Modifiers modifiers, int x, int y);
//  void new_mouse_click_event(MousePressEvent::Button button, InputEvent::Modifiers modifiers, int x, int y);

  friend class TerminalImpl;
  friend class InputParser;

private:
  bool read_input(const std::chrono::milliseconds &timeout, InputBuffer &into);

  void read_events() {
    this->input_parser.parse_event();
  }

  Terminal& write(const char* data, size_t size);

  friend Terminal& operator<<(Terminal &term, std::string_view const &value);
  friend Terminal& operator<<(Terminal &term, std::string const &value);
  friend Terminal& operator<<(Terminal &term, char value);
  friend Terminal& operator<<(Terminal &term, unsigned value);
  friend Terminal& operator<<(Terminal &term, signed value);

  friend class TerminalScreen;

  Terminal();
  ~Terminal();

public:
  struct Singleton {
    Singleton();
    ~Singleton();
  };

  static Terminal& get_singleton();

public:
  TerminalScreen& get_screen();

  Dimension get_size();

  void hide_cursor();
  void show_cursor(Cursor cursor = Cursor::DEFAULT);

  void set_title(const std::string &title);

  void flush();

  void run_event_loop() {
    screen.run_event_loop();
  }

  void post(std::function<void()> fn) {
    screen.post(std::move(fn));
  }
};

namespace detail {
static Terminal::Singleton terminal_singleton;
}

}
