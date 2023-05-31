#ifndef _WIN32

#include <csignal>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <tui++/Terminal.h>

static struct TerminalImpl {
  struct termios terminal;
  int input_flags;

  TerminalImpl() {
    ::tcgetattr(STDIN_FILENO, &this->terminal);

    terminal.c_lflag &= ~ICANON;
    terminal.c_lflag &= ~ECHO;
    terminal.c_cc[VMIN] = 0;
    terminal.c_cc[VTIME] = 0;

    // this->input_flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
    // ::fcntl(STDIN_FILENO, F_SETFL, this->input_flags | O_NONBLOCK);

    ::tcsetattr(STDIN_FILENO, TCSANOW, &this->terminal);

    std::signal();

    Terminal::init();
  }

  ~TerminalImpl() {
    Terminal::deinit();

    //::fcntl(STDIN_FILENO, F_GETFL, this->input_flags);

    tcsetattr(STDIN_FILENO, TCSANOW, &this->terminal);
  }
} terminal_impl;

Dimension Terminal::get_size() {
  winsize w {};
  if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0 or w.ws_col == 0 or w.ws_row == 0) {
    return {80, 24};
  }
  return {w.ws_col, w.ws_row};
}

void Terminal::read_events() {
  terminal_impl.read_events();
}

#endif
