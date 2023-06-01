#ifndef UNICODE
#error Must be compiled in UNICODE mode
#endif

#ifdef _WIN32

#include <tui++/Terminal.h>

#include <locale>
#include <cstring>
#include <codecvt>
#include <iostream>

#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace tui {

static struct TerminalImpl {
  HANDLE input_handle, output_handle;
  DWORD input_mode, output_mode;

  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;

  TerminalImpl() {
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

    Terminal::init();
  }

  ~TerminalImpl() {
    Terminal::deinit();

    ::SetConsoleMode(this->output_handle, this->output_mode);
    ::SetConsoleMode(this->input_handle, this->input_mode);
  }

  void read_events(const std::chrono::milliseconds &timeout) {
    if (::WaitForSingleObject(this->input_handle, timeout.count()) == WAIT_TIMEOUT) {
      Terminal::input_parser.timeout();
      return;
    }

    DWORD number_of_events = 0;
    if (not ::GetNumberOfConsoleInputEvents(this->input_handle, &number_of_events) or number_of_events == 0) {
      return;
    }

    std::vector<INPUT_RECORD> records { number_of_events };
    ::ReadConsoleInput(this->input_handle, records.data(), (DWORD) records.size(), &number_of_events);
    records.resize(number_of_events);

    char buffer[256];
    size_t size = 0;

    for (auto &&record : records) {
      switch (record.EventType) {
      case KEY_EVENT: {
        const auto &key_event = record.Event.KeyEvent;
        if (not key_event.bKeyDown) { // ignore KEY UP events
          continue;
        }

        auto bytes = this->converter.to_bytes(key_event.uChar.UnicodeChar);
        std::memcpy(&buffer[size], bytes.c_str(), bytes.size());
        size += bytes.size();
        break;
      }
      case WINDOW_BUFFER_SIZE_EVENT:
        Terminal::new_resize_event();
        break;
      case MENU_EVENT:
      case FOCUS_EVENT:
      case MOUSE_EVENT:
        // TODO(mauve): Implement later.
        break;
      }
    }

    if (size != 0) {
      Terminal::input_parser.parse(buffer, size);
    }
  }

} terminal_impl;

static Dimension get_default_size() {
  // The terminal size in VT100 was 80x24. It is still used nowadays by
  // default in many terminal emulator. That's a good choice for a fallback
  // value.
  return {80, 24};
}

Dimension Terminal::get_size() {
  CONSOLE_SCREEN_BUFFER_INFO csbi { };
  if (::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    return {csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1};
  }

  return get_default_size();
}

void Terminal::read_events() {
  terminal_impl.read_events(INPUT_TIMEOUT);
}

}

#endif
