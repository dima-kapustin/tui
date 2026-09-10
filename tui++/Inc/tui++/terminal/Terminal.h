#pragma once

#include <chrono>
#include <optional>
#include <variant>
#include <iostream>
#include <functional>
#include <memory>
#include <string_view>

#include <tui++/Event.h>
#include <tui++/Color.h>
#include <tui++/Cursor.h>
#include <tui++/Screen.h>
#include <tui++/Dimension.h>

namespace tui {

class Terminal;
extern Terminal &terminal;

class TerminalImpl;
class TextScreen;
class TextGraphics;
class SixelScreen;

class Terminal {
  // The terminal's own default colors: what a cell shows when the program
  // never set a color for it.
  struct DefaultColors {
    std::optional<Color> foreground;
    std::optional<Color> background;
  };

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
    USE_ALTERNATE_SCREEN_BUFFER = 1049,

    // DECSDM (Sixel Display Mode): when set, the text cursor stays at the
    // top-left corner of a sixel image after it is drawn. It is kept reset
    // (the default): some terminals (Windows Terminal among them) stop
    // following the cursor position for each image while the mode is set, so
    // repainted regions would pile up at the top-left of the screen. The
    // graphic screen instead keeps every image one cell short of the
    // bottom-right corner so the cursor advance after an image cannot leave
    // the visible area and scroll the buffer.
    SIXEL_DISPLAY_MODE = 80
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
    // A console read fills the whole buffer at once (the platform readers
    // only read when the buffer is empty), so it must hold an entire input
    // burst: a fast mouse flick can deliver hundreds of SGR report bytes, and
    // a paste far more. 256 bytes was small enough that a burst wrapped the
    // ring and overwrote its unparsed head.
    constexpr static size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t read_pos = 0, write_pos = 0;

  public:
    // Appends a byte. When the buffer is full the byte is dropped (the newest
    // input loses to the oldest unparsed input): the stream the parser sees
    // is never corrupted by a wrap, which would tear every sequence around
    // the wrap point.
    void put(char c) {
      auto next = (this->write_pos + 1) % BUFFER_SIZE;
      if (next == this->read_pos) {
        return;
      }
      this->buffer[this->write_pos] = c;
      this->write_pos = next;
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

    char consume(const std::chrono::milliseconds &timeout) {
      if (this->read_pos == this->write_pos) {
        if (not read_terminal_input(timeout)) {
          return 0;
        }
      }
      auto pos = this->read_pos;
      this->read_pos = (this->read_pos + 1) % BUFFER_SIZE;
      return this->buffer[pos];
    }

    char consume() {
      return consume(std::chrono::milliseconds::zero());
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

    void new_mouse_event(bool pressed);

    char get() {
      return this->reader.get();
    }

    // Peeks the next byte, waiting up to `timeout` for it when the input
    // buffer is empty (0 on timeout). Sequence parsers use this so a control
    // sequence that is split across reads is never torn: the byte is first
    // waited for, then consumed via consume() once it is known to be there.
    char get(const std::chrono::milliseconds &timeout) {
      return this->reader.get(timeout);
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

    // True when unparsed input bytes are already buffered (the terminal read
    // that filled the buffer was given a timeout, so buffered bytes are real
    // input and not a wait artifact).
    bool has_buffered_input() const {
      return this->reader.get_available() > 0;
    }
  };

  using Clock = std::chrono::steady_clock;

private:
  bool quit;
  std::string_view type = "text";

  // Escape-sequence output is accumulated here and written to the terminal in
  // one burst per flush() (once per frame). Writing each cell/sequence straight
  // to the tty makes the terminal redraw after every fragment, which is what
  // the user sees as a left-to-right "snake" on an edit, or a flickering blank
  // edge under a fast scroll.
  std::string output_buffer;

  // std::cout's streambuf as found at init(), before anything can replace it
  // (tests and PerfProbe install their own capture). flush() compares the
  // stream's current streambuf against this: while it still matches, no
  // capture is in place and the frame burst is written straight to the OS
  // output handle, skipping the stdio flush whose synchronous round trip into
  // the console driver dominates the cost of a repaint frame.
  std::streambuf *default_stdout_streambuf = nullptr;

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

  // The Device Attributes reply, queried once and shared by
  // query_graphics_support (sixel detection) and query_graphics_geometry
  // (the maxGraphicSize fallback), so the DA round-trip runs at most once.
  struct DeviceAttributes {
    bool answered = false;     // the terminal sent a DA reply
    bool has_graphics = false; // Primary DA Ps = 4/62, or Secondary DA Pp = 2/18/19/32
    bool is_xterm = false;     // Secondary DA with xterm's firmware signature (Pc = 0, Pv >= 95)
  };
  DeviceAttributes device_attributes;
  bool device_attributes_queried = false;

  DeviceAttributes query_device_attributes();

  DefaultColors default_colors;
  bool default_colors_queried = false;

private:
  void set_option(Option option);
  void reset_option(Option option);

  void init();
  void deinit();

  // The escape-sequence part of query_cell_size (shared by all platforms);
  // the platform files wrap it with their own fallback source.
  std::optional<Dimension> query_cell_size_from_terminal();

  void new_resize_event();

public:
  // -------------------------------------------------------------------
  // Input translation
  //
  // The input path is: platform input reader -> InputParser (bytes to event
  // parameters) -> the new_*_event entry points below (event parameters to
  // events on the screen, tracking the held-button/keyboard state in
  // between). The decode steps are pure and the entry points only touch the
  // screen queue and this terminal's input state, so both halves are
  // exercised in-process by the unit tests (tui++tests/test_InputTranslation)
  // instead of only through a live console.

  // The decoded content of one SGR mouse report, `ESC [ < code ; x ; y M`
  // (press/motion) or `... m` (release), mode 1006. `code`'s low two bits
  // carry the X11 button numbering the press reports use: 0 = left,
  // 1 = middle, 2 = right. Motion reports (bit 5, mode 1003) reuse the same
  // numbering for the button HELD during the motion, and code 3 -- the X10
  // release marker -- means no button is held, i.e. a plain move (xterm, the
  // Windows console and Windows Terminal all encode a buttonless move as
  // 32 + 3 = 35). Bit 6 marks a wheel report (64 + button, rotation -1 for
  // button 0 = wheel up, +1 otherwise). The modifier bits 2..4 (Shift = 4,
  // Alt/Meta = 8, Ctrl = 16) apply to every report kind.
  // `pressed` (the report terminator) selects PRESS over RELEASE for the
  // non-motion reports; motion and wheel reports ignore it.
  struct MouseReport {
    enum class Kind { PRESS, RELEASE, DRAG, MOVE, WHEEL };
    Kind kind = Kind::MOVE;
    MousePressEvent::Button button = MousePressEvent::NO_BUTTON;
    int wheel_rotation = 0;
    InputEvent::Modifiers key_modifiers = InputEvent::NO_MODIFIERS;
  };
  static MouseReport decode_mouse_report(unsigned code, bool pressed);

  // The modifiers of a modified key sequence, `ESC [ 1 ; <param> X`: xterm
  // encodes them as 1 + a bit mask (bit 0 = Shift, 1 = Alt, 2 = Ctrl,
  // 3 = Meta), so Ctrl+Up arrives as `ESC [ 1 ; 5 A`. Returns
  // NO_MODIFIERS when the sequence carries no modifier parameter.
  static InputEvent::Modifiers decode_csi_key_modifiers(std::vector<unsigned> const &params);

  // The key of a parameterized CSI key sequence `ESC [ params ; <selector>`
  // for the selectors that carry keys: the cursor keys 'A'..'D', 'H'/'F'
  // (Home/End) and the numbered '~' keys (`ESC [ 3 ~` = Delete,
  // `ESC [ 5 ; 5 ~` = Ctrl+PageUp). Returns nothing for selectors that are
  // not keys (the mouse reports 'M'/'m', the 'R' cursor position report, ...).
  static std::optional<KeyEvent::KeyCode> decode_csi_key(std::vector<unsigned> const &params, char selector);

  // Posts an input event on the screen queue, the way the parser does after
  // decoding one report/sequence. The keyboard entry points target the
  // focused window; the mouse ones retarget to the component under the
  // pointer (the window dispatcher does the hit-test). Application code
  // normally reaches these through the parser only.
  void new_key_event(const Char &c, InputEvent::Modifiers key_modifiers);
  void new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers key_modifiers);
  void new_mouse_event(MousePressEvent::Type type, MousePressEvent::Button button, InputEvent::Modifiers key_modifiers, int x, int y);
  void new_mouse_wheel_event(int wheel_rotation, InputEvent::Modifiers key_modifiers, int x, int y);
  void new_mouse_move_event(InputEvent::Modifiers modifiers, int x, int y);
  void new_mouse_drag_event(MousePressEvent::Button button, InputEvent::Modifiers modifiers, int x, int y);
//  void new_mouse_click_event(MousePressEvent::Button button, InputEvent::Modifiers modifiers, int x, int y);

private:
  friend class TerminalImpl;
  friend class InputParser;

private:
  bool read_input(const std::chrono::milliseconds &timeout, InputBuffer &into);

  // The modifiers the platform reports as currently held, best-effort. Used
  // to recover Shift (Ctrl/Meta) state that a mouse wheel report did not
  // encode, so Shift+wheel can scroll horizontally.
  InputEvent::Modifiers current_key_modifiers() const;

  void read_events() {
    // Parses every complete input sequence the terminal delivered since the
    // last tick, not just one. Mouse motion arrives as a burst of reports;
    // parsing (and dispatching) the whole burst in one tick lets its side
    // effects coalesce -- the repaint requests merge into the single pending
    // repaint invocation, so a burst costs one paint instead of one paint per
    // report. A report-per-tick drain would keep painting long after the
    // mouse stops (each paint takes tens of ms on a ConPTY terminal, and the
    // remaining buffered reports would each schedule their own). The cap
    // keeps a pathological paste or key autorepeat from starving the
    // dispatch loop.
    constexpr size_t MAX_INPUT_EVENTS_PER_TICK = 256;
    auto parsed = size_t { 0 };
    do {
      this->input_parser.parse_event();
    } while (++parsed < MAX_INPUT_EVENTS_PER_TICK and this->input_parser.has_buffered_input());
  }

  Terminal& write(const char *data, size_t size);

  // Writes a burst straight to the OS output handle, bypassing the stdio
  // stream (used when no capture is installed on std::cout). Defined in the
  // platform files: WriteFile to the console handle on Windows, write(2) on
  // POSIX.
  void write_direct(const char *data, size_t size);

  friend Terminal& operator<<(Terminal &term, std::string_view const &value);
  friend Terminal& operator<<(Terminal &term, std::string const &value);
  friend Terminal& operator<<(Terminal &term, char value);
  friend Terminal& operator<<(Terminal &term, unsigned value);
  friend Terminal& operator<<(Terminal &term, signed value);

  friend class TextScreen;
  friend class SixelScreen;

  Terminal();
  ~Terminal();

public:
  struct Singleton {
    Singleton();
    ~Singleton();
  };

  static Terminal& get_singleton();

public:
  Screen& get_screen();

  Dimension get_size();

  // Queries the terminal for its cell size in pixels: CSI 16 t (cell size)
  // with CSI 14 t / CSI 18 t (text area in pixels / characters) as fallback,
  // and finally the platform's own source (e.g. the console font size on
  // Windows). Returns an empty optional when the terminal does not answer
  // within a short timeout, in which case the caller falls back to its
  // defaults.
  std::optional<Dimension> query_cell_size();

  // Asks the terminal whether it supports sixel graphics, via the Primary
  // and Secondary Device Attributes. Returns true when the terminal reports
  // a graphics emulation (xterm adds Ps = 4 to its Primary DA when sixel is
  // enabled and identifies as VT240/VT330/VT340/VT382 in its Secondary DA),
  // false when it identifies itself as an xterm-style emulation without
  // graphics (so the caller can fail with an explanation instead of drawing
  // a black screen), and an empty optional when the terminal does not answer
  // or is unrecognized (Windows Terminal and others).
  std::optional<bool> query_graphics_support();

  // Asks the terminal for the largest sixel image it will display, in
  // pixels (XTSMGRAPHICS: `CSI ? 2 ; 4 S` reads the maximum allowed geometry;
  // xterm's maxGraphicSize resource, default 1000x1000). The sixel screen
  // tiles every flushed image to at most this size so nothing is truncated:
  // xterm silently drops the right and bottom of any larger image. xterm
  // builds that predate XTSMGRAPHICS answer nothing; they are identified by
  // their Device Attributes and get the default 1000x1000. Terminals with
  // no such limit (Windows Terminal) get an empty optional.
  std::optional<Dimension> query_graphics_geometry();

  // Selects the rendering backend: "char" for the escape-sequence terminal
  // screen (the default) or "sixel" for the pixel-level sixel screen. The
  // screen is created implicitly the first time it is requested.
  void set_type(std::string_view type);

  // Asks the terminal for its default foreground and background colors, via
  // the xterm controls OSC 10 ; ? and OSC 11 ; ? (Windows Terminal and
  // others answer them too; the Linux console and older terminals never do).
  // A color is empty when the terminal does not answer within a short
  // timeout, and the round-trip runs at most once -- the answers are cached.
  DefaultColors query_default_colors();

  // The color a terminal color report names: OSC 10/11 answer with
  // "rgb:RRRR/GGGG/BBBB" (xterm sends four hex digits per component, other
  // terminals two or one) or "#RRGGBB". The components are scaled from
  // however many digits the report carries; anything else has no color.
  static std::optional<Color> parse_color_spec(std::string_view const &spec);

  void hide_cursor();
  void show_cursor(std::optional<Cursor> const &cursor = { });

  void set_title(const std::string &title);

  // Escape sequence that undoes every piece of terminal state init()
  // changes: mouse reporting (?1000h ?1003h ?1015h ?1006h), the alternate
  // screen buffer (?1049h), the hidden cursor (?25l), DECSDM (?80l) and
  // line wrap (?7l). deinit() sends it on a normal exit; the platform
  // signal handlers write it directly when the process dies abnormally
  // (Ctrl+C, a fatal signal or an uncaught exception), so the terminal is
  // restored even when no destructor runs.
  static constexpr std::string_view RESTORE_SEQUENCE = "\x1b[?1006l\x1b[?1015l\x1b[?1003l\x1b[?1000l\x1b[?1049l\x1b[?80h\x1b[?25h\x1b[?7h";

  void flush();

  void run_event_loop() {
    screen.run_event_loop();
  }

  // Requests the event loop to finish on its next iteration. Components may
  // call this to shut the application down cleanly (the screen keeps
  // draining the remaining queued events, then run_event_loop returns and
  // the terminal state is restored on destruction).
  void shutdown() {
    screen.quit = true;
  }

  void post(std::function<void()> fn) {
    screen.post(std::move(fn));
  }
};

namespace detail {
static Terminal::Singleton terminal_singleton;
}

}
