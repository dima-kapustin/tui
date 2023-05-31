#pragma once

#include <chrono>
#include <functional>

#include <tui++/Dimension.h>
#include <tui++/EventQueue.h>

namespace tui {

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

    ALTERNATE_SCREEN = 1049
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

  constexpr static std::chrono::milliseconds INPUT_TIMEOUT { 50 };

  struct InputParser {
    enum class State {
      INIT,
      SKIP,

      ESC,

      SS3, // ESC O Single Shift Select of G3 Character Set
      DCS, // ESC P Device Control String
      OSC, // ESC ] Operating System Command
      CSI, // ESC [ Control Sequence Introducer
      CSI_ARGS,
      CSI_SELECTOR
    };

    class Input {
      const std::string &input;
      size_t pos;

    public:
      Input(const std::string &input) :
          input(input), pos(0) {
      }

      Input& operator=(const Input&) = delete;

    public:
      operator char() const {
        return this->pos < this->input.size() ? this->input[this->pos] : 0;
      }

      char consume() {
        char c = *this;
        this->pos += 1;
        return c;
      }
    };

  private:
    State state = State::INIT;

    std::vector<unsigned> csi_args;
    bool csi_altered = false;

  private:
    void parse(Input &input);
    void parse_esc(Input &input);

    void parse_ss3(Input &input);
    void parse_dcs(Input &input);
    void parse_csi(Input &input);
    void parse_csi_args(Input &input);
    void parse_csi_selector(Input &input);
    void parse_osc(Input &input);

  private:
    void reset_csi() {
      this->csi_args.resize(1);
      this->csi_args[0] = 0;
      this->csi_altered = false;
    }

  private:
    void new_key_event(KeyEvent::KeyCode key_code) {
      Terminal::new_key_event(key_code, { });
      reset();
    }

    void new_mouse_event(bool pressed) {
      auto button = this->csi_args[0] & 3;
      auto type = this->csi_args[0] & 64 ? MouseEvent::MOUSE_WHEEL : (pressed ? MouseEvent::MOUSE_PRESSED : MouseEvent::MOUSE_RELEASED);
      int modifiers = this->csi_args[0] & 4 ? InputEvent::Modifiers::SHIFT_MASK : InputEvent::Modifiers::NONE;
      modifiers |= this->csi_args[0] & 8 ? InputEvent::Modifiers::META_MASK : InputEvent::Modifiers::NONE;
      modifiers |= this->csi_args[0] & 16 ? InputEvent::Modifiers::CTRL_MASK : InputEvent::Modifiers::NONE;
      int x = this->csi_args[1];
      int y = this->csi_args[2];
      Terminal::new_mouse_event(type, MouseEvent::Button(button), InputEvent::Modifiers(modifiers), x, y);
      reset();
    }

  public:
    void parse(const std::string &seq) {
      auto input = Input { seq };
      parse(input);
    }

    void reset() {
      this->state = State::INIT;
    }

    void timeout() {
      if (this->state == State::ESC) {
        Terminal::new_key_event(KeyEvent::VK_ESCAPE, { });
      }
      reset();
    }
  };

  using Clock = std::chrono::steady_clock;

private:
  static bool quit;
  static EventQueue event_queue;
  static InputParser input_parser;

  static MouseEvent prev_mouse_event;
  static Clock::time_point prev_mouse_press_time;
  static Clock::time_point prev_mouse_click_time;

  static std::chrono::milliseconds mouse_click_detection_timeout;
  static std::chrono::milliseconds mouse_double_click_detection_timeout;

  static std::vector<Option> set_options;

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
  static void read_events();

public:
  static Dimension get_size();

  static void run_event_loop();

  static void post(std::function<void()> fn);

  static void flush();
};

}
