#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalScreen.h>
#include <tui++/terminal/TerminalGraphics.h>

#include <tui++/util/utf-8.h>

#include <iostream>

namespace tui::detail {
Screen& get_screen() {
  return terminal::Terminal::get_singleton().get_screen();
}
}

namespace tui::terminal {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

static void escape_attrs(std::ostream &os, const Attributes &reset, const Attributes &set) {
  auto reset_pos = os.tellp();
  for (auto &&attr : reset) {
    if (reset_pos == os.tellp()) {
      os << "\x1B[";
    } else {
      os << ';';
    }
    switch (attr) {
    case Attribute::INVERSE:
    case Attribute::STANDOUT:
      os << "27";
      break;
    case Attribute::BOLD:
    case Attribute::DIM:
      os << "22";
      break;
    case Attribute::ITALIC:
      os << "23";
      break;
    case Attribute::UNDERLINE:
    case Attribute::DOUBLE_UNDERLINE:
      os << "24";
      break;
    case Attribute::BLINK:
      os << "25";
      break;
    case Attribute::INVISIBLE:
      os << "28";
      break;
    case Attribute::CROSSED_OUT:
      os << "29";
      break;

    default:
      break;
    }
  }

  auto set_pos = os.tellp();
  for (auto &&attr : set) {
    if (reset_pos == os.tellp()) {
      os << "\x1B[";
    } else if (set_pos != os.tellp()) {
      os << ';';
    }
    switch (attr) {
    case Attribute::STANDOUT:
      os << "1;7";
      break;
    case Attribute::BOLD:
      os << '1';
      break;
    case Attribute::DIM:
      os << '2';
      break;
    case Attribute::ITALIC:
      os << '3';
      break;
    case Attribute::UNDERLINE:
      os << '4';
      break;
    case Attribute::BLINK:
      os << '5';
      break;
    case Attribute::INVERSE:
      os << '7';
      break;
    case Attribute::INVISIBLE:
      os << '8';
      break;
    case Attribute::CROSSED_OUT:
      os << '9';
      break;
    case Attribute::DOUBLE_UNDERLINE:
      os << "21";
      break;

    default:
      break;
    }
  }

  if (reset_pos != os.tellp()) {
    os << 'm';
  }
}

static void escape_background_color(std::ostream &os, const Color &color) {
  struct SetBackgroundColor {
    std::ostream &os;

    void operator()(const DefaultColor&) {
      this->os << "\x1b[49m";
    }

    void operator()(const ColorIndex &c) {
      this->os << "\x1b[48;5;" << c.value << 'm';
    }

    void operator()(const TrueColor &c) {
      this->os << "\x1b[48;2;" << c.red << ';' << c.green << ';' << c.blue << 'm';
    }
  };
  std::visit(SetBackgroundColor { os }, color);
}

static void escape_foreground_color(std::ostream &os, const Color &color) {
  struct SetForegroundColor {
    std::ostream &os;

    void operator()(const DefaultColor&) {
      this->os << "\x1b[39m";
    }

    void operator()(const ColorIndex &c) {
      this->os << "\x1b[38;5;" << c.value << 'm';
    }

    void operator()(const TrueColor &c) {
      this->os << "\x1b[38;2;" << c.red << ';' << c.green << ';' << c.blue << 'm';
    }
  };
  std::visit(SetForegroundColor { os }, color);
}

TerminalScreen::CharView TerminalScreen::EMPTY_CHAR_VIEW;

void TerminalScreen::run_event_loop() {
  event_dispatching_thread_id = std::this_thread::get_id();

  while (not this->quit) {
    this_terminal.read_events();
    if (auto event = this->event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      dispatch_event(*event);
    }
  }
}

void TerminalScreen::resize_view() {
  auto size = this->size;
  this->size = this_terminal.get_size();
  if (size != this->size) {
    this->view.resize(this->size.height);
    for (auto &&row : this->view) {
      row.resize(this->size.width);
      std::fill(row.begin(), row.end(), EMPTY_CHAR_VIEW);
    }
    refresh();
  }
}

void TerminalScreen::terminal_resized() {
  resize_view();
}

void TerminalScreen::refresh() {
  resize_view();
  auto g = TerminalGraphics { *this };
  paint(g);
  flush();
}

std::string TerminalScreen::to_string() const {
  std::string buffer;
  buffer.reserve(4 * get_width() * get_height() + get_height());
  std::ostringstream os(buffer);

  const auto *prev_cv = &EMPTY_CHAR_VIEW;

  auto escape_attrs_and_colors = [&](const CharView &cv) {
    auto reset = prev_cv->attributes & ~cv.attributes;
    auto set = ~prev_cv->attributes & cv.attributes;

    escape_attrs(os, reset, set);

    if (prev_cv->background_color != cv.background_color) {
      escape_background_color(os, cv.background_color);
    }

    if (prev_cv->foreground_color != cv.foreground_color) {
      escape_foreground_color(os, cv.foreground_color);
    }

    prev_cv = &cv;
  };

  for (auto y = 0U; y < this->view.size(); ++y) {
    if (y) {
      escape_attrs_and_colors(EMPTY_CHAR_VIEW);
      os << "\r\n";
    }

    auto skip = false;
    for (auto &&cv : this->view[y]) {
      if (not skip) {
        escape_attrs_and_colors(cv);
        os << cv.ch;
      }
      skip = cv.is_wide();
    }
  }

  escape_attrs_and_colors(EMPTY_CHAR_VIEW);

  return os.str();
}

void TerminalScreen::clear() {
  for (auto &line : this->view) {
    for (auto &cell : line) {
      cell = EMPTY_CHAR_VIEW;
    }
  }
}

void TerminalScreen::flush() const {
  this_terminal.move_cursor_to(1, 1);
  this_terminal.print(to_string());
  this_terminal.flush();
}

}
