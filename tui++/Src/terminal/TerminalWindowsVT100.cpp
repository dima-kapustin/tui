#ifdef _WIN32

#ifndef UNICODE
#error Must be compiled in UNICODE mode
#endif

#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalScreen.h>

#include <locale>
#include <cstring>
#include <iostream>

#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace tui::terminal {

class TerminalImpl {
  Terminal &terminal;

  HANDLE input_handle, output_handle;
  DWORD input_mode, output_mode;

  std::vector<INPUT_RECORD> input_records { 16 };

public:
  TerminalImpl(Terminal &terminal) :
      terminal(terminal) {
    // Enable VT processing on stdout and stdin
    this->output_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    this->input_handle = ::GetStdHandle(STD_INPUT_HANDLE);

    ::GetConsoleMode(this->input_handle, &this->input_mode);
    auto input_mode = (this->input_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)) | (ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT);
    ::SetConsoleMode(this->input_handle, input_mode);

    ::GetConsoleMode(this->output_handle, &this->output_mode);
    auto output_mode = this->output_mode | (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    ::SetConsoleMode(this->output_handle, output_mode);

    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
  }

  bool read_input(const std::chrono::milliseconds &timeout, Terminal::InputBuffer &into) {
    if (::WaitForSingleObject(this->input_handle, timeout.count()) == WAIT_TIMEOUT) {
      return false;
    }

    DWORD number_of_events = 0;
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
      case MENU_EVENT:
      case FOCUS_EVENT:
      case MOUSE_EVENT:
        // TODO(mauve): Implement later.
        break;
      }
    }

    return was_available != into.get_available();
  }

  ~TerminalImpl() {
    ::SetConsoleMode(this->output_handle, this->output_mode);
    ::SetConsoleMode(this->input_handle, this->input_mode);
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

}

#endif
