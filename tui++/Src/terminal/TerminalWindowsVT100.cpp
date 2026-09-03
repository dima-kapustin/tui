#ifdef _WIN32

#ifndef UNICODE
#error Must be compiled in UNICODE mode
#endif

#include <tui++/terminal/Terminal.h>

#include <csignal>
#include <locale>
#include <cstring>
#include <iostream>

#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace tui {

namespace {

// The console settings this process changed, saved so that an abnormal exit
// can still put them back. A normal exit runs TerminalImpl's destructor, but
// Ctrl+C, Ctrl+Break, abort() or a crash never will, so the console control
// handler and the CRT signal handlers below restore this state instead.
struct ConsoleState {
  HANDLE input = nullptr;
  HANDLE output = nullptr;
  DWORD input_mode = 0;
  DWORD output_mode = 0;
  UINT input_cp = 0;
  UINT output_cp = 0;
  volatile LONG restored = 0;
};

ConsoleState console_state;

// Puts the console back the way it was found: console modes, code pages and
// (when the caller asks) the terminal state changed by the escape sequences.
// Idempotent: whichever of the destructor and the handlers runs first wins.
void restore_console(bool emit_sequence) {
  if (InterlockedExchange(&console_state.restored, 1)) {
    return;
  }
  if (console_state.input) {
    ::SetConsoleMode(console_state.input, console_state.input_mode);
    // Discard whatever the app never read -- trailing mouse-tracking key
    // events, replies to the startup queries, the keypress that quit.
    // Without this the shell reads them after the app exits and types
    // control characters into the prompt.
    ::FlushConsoleInputBuffer(console_state.input);
  }
  if (console_state.output) {
    ::SetConsoleMode(console_state.output, console_state.output_mode);
    if (emit_sequence) {
      auto &seq = Terminal::RESTORE_SEQUENCE;
      auto written = DWORD { };
      ::WriteFile(console_state.output, seq.data(), DWORD(seq.size()), &written, nullptr);
    }
  }
  if (console_state.input_cp) {
    ::SetConsoleCP(console_state.input_cp);
    ::SetConsoleOutputCP(console_state.output_cp);
  }
}

// Ctrl+C and Ctrl+Break arrive as console control events on a dedicated
// thread, so the full restore (including the escape sequences) is safe here.
BOOL WINAPI console_ctrl_handler(DWORD type) {
  switch (type) {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
    restore_console(true);
    ::ExitProcess(130); // 128 + SIGINT, matching POSIX convention
  case CTRL_CLOSE_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    // The console is already going away; let the default handling terminate
    // the process (there is no terminal left to restore into).
    return FALSE;
  }
  return FALSE;
}

// abort(), std::terminate (an uncaught exception) and raise() come through
// the CRT signals; restore, then re-raise with the default disposition so
// the exit status still reflects the original signal.
void crt_signal_handler(int sig) {
  restore_console(true);
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}

}

class TerminalImpl {
  Terminal &terminal;

  HANDLE input_handle, output_handle;
  DWORD input_mode, output_mode;
  UINT input_cp, output_cp; // code pages

  std::vector<INPUT_RECORD> input_records { 16 };

public:
  TerminalImpl(Terminal &terminal) :
      terminal(terminal) {
    // Enable VT processing on stdout and stdin
    this->output_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    this->input_handle = ::GetStdHandle(STD_INPUT_HANDLE);

    ::GetConsoleMode(this->input_handle, &this->input_mode);
    auto input_mode = (this->input_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_QUICK_EDIT_MODE)) | (ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS);
    ::SetConsoleMode(this->input_handle, input_mode);

    ::GetConsoleMode(this->output_handle, &this->output_mode);
    auto output_mode = this->output_mode | (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    ::SetConsoleMode(this->output_handle, output_mode);

    this->input_cp = ::GetConsoleCP();
    this->output_cp = ::GetConsoleOutputCP();
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".utf8");

    // From here on, even a Ctrl+C, abort() or crash restores the console:
    // the handlers run when the process dies abnormally, before any
    // destructor would get the chance.
    console_state = { this->input_handle, this->output_handle, this->input_mode, this->output_mode, this->input_cp, this->output_cp, 0 };
    ::SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    std::signal(SIGINT, crt_signal_handler);
    std::signal(SIGTERM, crt_signal_handler);
    std::signal(SIGABRT, crt_signal_handler);
    std::signal(SIGSEGV, crt_signal_handler);
  }

  bool read_input(const std::chrono::milliseconds &timeout, Terminal::InputBuffer &into) {
    if (::WaitForSingleObject(this->input_handle, timeout.count()) == WAIT_TIMEOUT) {
      return false;
    }

    auto number_of_events = DWORD { };
    if (not ::GetNumberOfConsoleInputEvents(this->input_handle, &number_of_events) or number_of_events == 0) {
      return false;
    }

    this->input_records.resize(number_of_events);
    ::ReadConsoleInput(this->input_handle, this->input_records.data(), this->input_records.size(), &number_of_events);
    this->input_records.resize(number_of_events);

    auto was_available = into.get_available();
    auto wstring = std::wstring { };
    for (auto &&record : this->input_records) {
      switch (record.EventType) {
      case KEY_EVENT: {
        if (auto const &key_event = record.Event.KeyEvent; not key_event.bKeyDown) { // ignore KEY UP events
          continue;
        } else if (auto wc = key_event.uChar.UnicodeChar) {
          if (wc >= 0xD800 and wc <= 0xDBFF) {
            // Wait for the Low Surrogate to arrive in the next record.
            wstring.reserve(2); // assuming Small String Optimization (SSO)
            wstring.clear();
            wstring += wc;
            continue;
          } else if (wstring.empty()) {
            for (auto &&c : util::to_utf8(wc)) {
              into.put(c);
            }
          } else {
            wstring += wc;
            for (auto &&c : util::to_utf8(wstring)) {
              into.put(c);
            }
            wstring.clear();
          }
        }
        break;
      }
      case WINDOW_BUFFER_SIZE_EVENT:
        this->terminal.new_resize_event();
        break;
      }
    }

    return was_available != into.get_available();
  }

  std::optional<Dimension> query_cell_size() {
    // The console font size is the terminal cell size in pixels (TrueType
    // fonts); used when the terminal ignores the escape-sequence queries.
    CONSOLE_FONT_INFO info { };
    if (::GetCurrentConsoleFont(this->output_handle, FALSE, &info) and info.dwFontSize.X > 0 and info.dwFontSize.Y > 0) {
      auto width = int(info.dwFontSize.X);
      auto height = int(info.dwFontSize.Y);
      if (width >= 4 and width <= 64 and height >= 4 and height <= 64) {
        return Dimension { width, height };
      }
    }
    return { };
  }

  ~TerminalImpl() {
    // Remove the handlers first so a signal arriving during teardown does
    // not race the restore below. deinit() has already sent the escape
    // sequences, so only the console modes and code pages are restored.
    ::SetConsoleCtrlHandler(console_ctrl_handler, FALSE);
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    std::signal(SIGABRT, SIG_DFL);
    std::signal(SIGSEGV, SIG_DFL);
    restore_console(false);
  }
};

Terminal::Terminal() :
    impl(std::make_unique<TerminalImpl>(*this)) {
  init();
}

Terminal::~Terminal() {
  deinit();
}

bool Terminal::read_input(const std::chrono::milliseconds &timeout, InputBuffer &into) {
  return this->impl->read_input(timeout, into);
}

static Dimension get_default_size() {
  // The terminal size in VT100 was 80x24.
  // It is still used nowadays by default in many terminal emulators.
  // That's a good choice for a fallback value.
  return {80, 24};
}

Dimension Terminal::get_size() {
  CONSOLE_SCREEN_BUFFER_INFO csbi { };
  if (::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    return {csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1};
  }

  return get_default_size();
}

std::optional<Dimension> Terminal::query_cell_size() {
  if (auto size = query_cell_size_from_terminal()) {
    return size;
  }
  return this->impl->query_cell_size();
}

}

#endif
