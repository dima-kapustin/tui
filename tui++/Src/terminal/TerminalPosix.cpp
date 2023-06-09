#ifndef _WIN32

#include <csignal>

#include <sys/ioctl.h>
#include <sys/select.h>

#include <termios.h>
#include <unistd.h>

#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalScreen.h>

namespace tui::terminal {

struct TerminalImpl {
  static inline TerminalImpl *impl = nullptr;
  Terminal &terminal;

  struct ::termios termios;
  //int input_flags;

public:
  TerminalImpl(Terminal &terminal) :
      terminal(terminal) {
    impl = this;

    ::tcgetattr(STDIN_FILENO, &this->termios);
    auto termios = this->termios;
    termios.c_lflag &= ~(ICANON|ECHO|ICRNL);
    termios.c_cc[VMIN] = 1;
    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);

    //this->input_flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
    //::fcntl(STDIN_FILENO, F_SETFL, this->input_flags | O_NONBLOCK);

    std::signal(SIGWINCH, signal_handler);
  }

  bool is_stdin_empty(const std::chrono::microseconds &timeout) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv = {0, timeout.count()};
    ::select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    return not FD_ISSET(STDIN_FILENO, &fds);
  }

  bool read_input(const std::chrono::milliseconds &timeout, Terminal::InputBuffer &into) {
    if (is_stdin_empty(timeout)) {
      return false;
    }
    char byte = '\0';
    if ((::read(fileno(stdin), &byte, 1) == 1)) {
      into.put(byte);
      return true;
    }
    return false;
  }

  ~TerminalImpl() {
    //::fcntl(STDIN_FILENO, F_GETFL, this->input_flags);

    ::tcsetattr(STDIN_FILENO, TCSANOW, &this->termios);
  }

  static void signal_handler(int signal) {
    switch(signal) {
    case SIGWINCH:
        impl->terminal.new_resize_event();
        break;
    }
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

Dimension Terminal::get_size() {
  winsize w {};
  if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0 or w.ws_col == 0 or w.ws_row == 0) {
      return {80, 24};
  }
  return {w.ws_col, w.ws_row};
}

}

#endif
