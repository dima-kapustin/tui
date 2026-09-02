#ifndef _WIN32

#include <csignal>

#include <sys/ioctl.h>
#include <sys/select.h>

#include <termios.h>
#include <unistd.h>

#include <tui++/terminal/Terminal.h>

namespace tui {

struct TerminalImpl;

namespace {

// Defined after TerminalImpl: they need its saved termios.
void restore_terminal_state();
void fatal_signal_handler(int sig);
void install_fatal_signal_handlers();
void uninstall_fatal_signal_handlers();

}

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

    // From here on, even a Ctrl+C, a fatal signal or a crash restores the
    // terminal: the handlers run when the process dies abnormally, before
    // any destructor would get the chance.
    install_fatal_signal_handlers();
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
    if (not is_stdin_empty(timeout)) {
      auto byte = '\0';
      if ((::read(fileno(stdin), &byte, 1) == 1)) {
        into.put(byte);
        return true;
      }
    }
    return false;
  }

  std::optional<Dimension> query_cell_size() {
    return { }; // no platform-specific cell-size source on POSIX
  }

  ~TerminalImpl() {
    // Remove the handlers first so a signal arriving during teardown does
    // not race the restore below; deinit() has already sent the escape
    // sequences.
    uninstall_fatal_signal_handlers();

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

namespace {

// Restores the terminal when the process dies abnormally: a signal, a crash
// or an uncaught exception never runs destructors, so these handlers put the
// line discipline and the escape-sequence state back from the signal context
// (everything used below is async-signal-safe), then re-raise the signal
// with its default disposition so the exit status and core dump still
// reflect what actually happened.
volatile sig_atomic_t restore_in_progress = 0;

constexpr int FATAL_SIGNALS[] = { SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGABRT, SIGSEGV, SIGFPE, SIGBUS, SIGILL };
struct sigaction saved_actions[sizeof(FATAL_SIGNALS) / sizeof(FATAL_SIGNALS[0])] { };

void restore_terminal_state() {
  if (restore_in_progress) {
    return;
  }
  restore_in_progress = 1;
  if (TerminalImpl::impl) {
    ::tcsetattr(STDIN_FILENO, TCSANOW, &TerminalImpl::impl->termios);
  }
  auto &seq = Terminal::RESTORE_SEQUENCE;
  ::write(STDOUT_FILENO, seq.data(), seq.size());
}

void fatal_signal_handler(int sig) {
  restore_terminal_state();
  struct sigaction action { };
  action.sa_handler = SIG_DFL;
  ::sigemptyset(&action.sa_mask);
  ::sigaction(sig, &action, nullptr);
  ::raise(sig);
}

void install_fatal_signal_handlers() {
  for (auto i = 0u; i < sizeof(FATAL_SIGNALS) / sizeof(FATAL_SIGNALS[0]); ++i) {
    struct sigaction action { };
    action.sa_handler = fatal_signal_handler;
    ::sigemptyset(&action.sa_mask);
    ::sigaction(FATAL_SIGNALS[i], &action, &saved_actions[i]);
  }
}

void uninstall_fatal_signal_handlers() {
  for (auto i = 0u; i < sizeof(FATAL_SIGNALS) / sizeof(FATAL_SIGNALS[0]); ++i) {
    ::sigaction(FATAL_SIGNALS[i], &saved_actions[i], nullptr);
  }
}

}

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

std::optional<Dimension> Terminal::query_cell_size() {
  return query_cell_size_from_terminal();
}

}

#endif
