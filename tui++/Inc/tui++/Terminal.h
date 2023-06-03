#pragma once

#include <chrono>
#include <variant>
#include <functional>

#include <tui++/Event.h>
#include <tui++/Dimension.h>

namespace tui {

class Terminal {
  static std::chrono::milliseconds read_terminal_input_timeout;

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

  class InputReader {
    constexpr static size_t BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    size_t read_pos = 0, write_pos = 0;

  private:
    void put(char c) {
      this->buffer[this->write_pos] = c;
      this->write_pos = (this->write_pos + 1) % BUFFER_SIZE;
    }

    bool read_terminal_input(const std::chrono::milliseconds &timeout);

  public:
    char get() {
      return get(std::chrono::milliseconds::zero());
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
        if(not read_terminal_input(std::chrono::milliseconds::zero())) {
          return 0;
        }
      }
      auto pos = this->read_pos;
      this->read_pos = (this->read_pos + 1) % BUFFER_SIZE;
      return this->buffer[pos];
    }
  };

  struct InputParser {
    enum class State {
      INIT,
      SKIP,

      ESC,

      SS3, // ESC O Single Shift Select of G3 Character Set
      DCS, // ESC P Device Control String
      OSC, // ESC ] Operating System Command

      CSI, // ESC [ Control Sequence Introducer
      CSI_PARAMS,
      CSI_SELECTOR,

      UTF8
    };

    InputReader reader;

  private:
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
    void new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers = InputEvent::NONE_MASK) {
      Terminal::new_key_event(key_code, modifiers);
    }

    void new_mouse_event(bool pressed) {
      auto button = this->csi_params[0] & 3;
      auto type = this->csi_params[0] & 64 ? MouseEvent::MOUSE_WHEEL : (pressed ? MouseEvent::MOUSE_PRESSED : MouseEvent::MOUSE_RELEASED);
      int modifiers = this->csi_params[0] & 4 ? InputEvent::Modifiers::SHIFT_MASK : InputEvent::Modifiers::NONE_MASK;
      modifiers |= this->csi_params[0] & 8 ? InputEvent::Modifiers::META_MASK : InputEvent::Modifiers::NONE_MASK;
      modifiers |= this->csi_params[0] & 16 ? InputEvent::Modifiers::CTRL_MASK : InputEvent::Modifiers::NONE_MASK;
      int x = this->csi_params[1];
      int y = this->csi_params[2];
      Terminal::new_mouse_event(type, MouseEvent::Button(button), InputEvent::Modifiers(modifiers), x, y);
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
    void parse_event();
  };

  using Clock = std::chrono::steady_clock;

private:
  static bool quit;
  static InputParser input_parser;

  static std::vector<Option> set_options;

  static MouseEvent prev_mouse_event;
  static Clock::time_point prev_mouse_press_time;
  static Clock::time_point prev_mouse_click_time;

  static std::chrono::milliseconds mouse_click_detection_timeout;
  static std::chrono::milliseconds mouse_double_click_detection_timeout;

private:
  static void set_option(Option option);
  static void reset_option(Option option);

  static void init();
  static void deinit();

  static void new_resize_event();
  static void new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers);
  static void new_mouse_event(MouseEvent::Type type, MouseEvent::Button button, InputEvent::Modifiers modifiers, int x, int y);

  friend struct TerminalImpl;
  friend struct InputParser;

private:
  static void read_events() {
    input_parser.parse_event();
  }

  friend class Screen;

public:
  static Dimension get_size();

  static void flush();

  static void set_title(const std::string &title);
};

}
