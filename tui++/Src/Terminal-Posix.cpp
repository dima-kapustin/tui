#ifndef _WIN32

#include <csignal>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <tui++/Terminal.h>

namespace tui {

static struct TerminalImpl {
    struct termios terminal;
    //int input_flags;

    TerminalImpl() {
        ::tcgetattr(STDIN_FILENO, &this->terminal);
        terminal.c_lflag &= ~(ICANON|ECHO);
        terminal.c_cc[VMIN] = 1;
        ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->terminal);

        //this->input_flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
        //::fcntl(STDIN_FILENO, F_SETFL, this->input_flags | O_NONBLOCK);

        std::signal(SIGWINCH, signal_handler);

        Terminal::init();
    }

    ~TerminalImpl() {
        Terminal::deinit();

        //::fcntl(STDIN_FILENO, F_GETFL, this->input_flags);

        ::tcsetattr(STDIN_FILENO, TCSANOW, &this->terminal);
    }

    static void signal_handler(int signal) {
        switch(signal) {
        case SIGWINCH:
            Terminal::new_resize_event();
            break;
        }
    }

    bool is_stdin_empty(const std::chrono::microseconds &timeout) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        timeval tv = {0, timeout.count()};
        ::select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
        return not FD_ISSET(STDIN_FILENO, &fds);
    }

    void read_events(const std::chrono::milliseconds &timeout) {
      constexpr size_t BUFFER_SIZE = 256;
      char buffer[BUFFER_SIZE];
      size_t size = 0;
      while (not is_stdin_empty(timeout)) {
        if (::read(fileno(stdin), buffer[size], 1) != 1) {
            // TODO report error
          break;
        }
        size += 1;
      }

      if (size != 0) {
        Terminal::input_parser.parse(buffer, size);
      }
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
    terminal_impl.read_events(read_event_timeout);
}

}

#endif
