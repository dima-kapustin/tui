#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalScreen.h>
#include <tui++/terminal/TerminalGraphics.h>

#include <tui++/util/utf-8.h>

#include <string_view>

namespace tui::detail {
Screen& get_screen() {
  return Terminal::get_singleton().get_screen();
}
}

using namespace std::string_view_literals;

namespace tui {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

static void escape_attrs(const Attributes &reset, const Attributes &set) {
  if (reset or set) {
    terminal << "\x1B["sv;
    auto delim = false;
    for (auto &&attr : reset) {
      if (delim) {
        terminal << ';';
      }
      switch (attr) {
      case Attribute::INVERSE:
      case Attribute::STANDOUT:
        terminal << "27"sv;
        break;
      case Attribute::BOLD:
      case Attribute::DIM:
        terminal << "22"sv;
        break;
      case Attribute::ITALIC:
        terminal << "23"sv;
        break;
      case Attribute::UNDERLINE:
      case Attribute::DOUBLE_UNDERLINE:
        terminal << "24"sv;
        break;
      case Attribute::BLINK:
        terminal << "25"sv;
        break;
      case Attribute::INVISIBLE:
        terminal << "28"sv;
        break;
      case Attribute::CROSSED_OUT:
        terminal << "29"sv;
        break;

      default:
        delim = false;
        continue;
      }

      delim = true;
    }

    for (auto &&attr : set) {
      if (delim) {
        terminal << ';';
      }
      switch (attr) {
      case Attribute::STANDOUT:
        terminal << "1;7"sv;
        break;
      case Attribute::BOLD:
        terminal << '1';
        break;
      case Attribute::DIM:
        terminal << '2';
        break;
      case Attribute::ITALIC:
        terminal << '3';
        break;
      case Attribute::UNDERLINE:
        terminal << '4';
        break;
      case Attribute::BLINK:
        terminal << '5';
        break;
      case Attribute::INVERSE:
        terminal << '7';
        break;
      case Attribute::INVISIBLE:
        terminal << '8';
        break;
      case Attribute::CROSSED_OUT:
        terminal << '9';
        break;
      case Attribute::DOUBLE_UNDERLINE:
        terminal << "21"sv;
        break;

      default:
        delim = false;
        continue;
      }

      delim = true;
    }

    terminal << 'm';
  }
}

static void escape_background_color(const Color &color) {
  struct SetBackgroundColor {
    void operator()(const DefaultColor&) {
      terminal << "\x1b[49m"sv;
    }

    void operator()(const ColorIndex &c) {
      terminal << "\x1b[48;5;"sv << c.value << 'm';
    }

    void operator()(const TrueColor &c) {
      terminal << "\x1b[48;2;"sv << c.red << ';' << c.green << ';' << c.blue << 'm';
    }
  };
  std::visit(SetBackgroundColor { }, color);
}

static void escape_foreground_color(const Color &color) {
  struct SetForegroundColor {
    void operator()(const DefaultColor&) {
      terminal << "\x1b[39m"sv;
    }

    void operator()(const ColorIndex &c) {
      terminal << "\x1b[38;5;"sv << c.value << 'm';
    }

    void operator()(const TrueColor &c) {
      terminal << "\x1b[38;2;"sv << c.red << ';' << c.green << ';' << c.blue << 'm';
    }
  };
  std::visit(SetForegroundColor { }, color);
}

TerminalScreen::CharView TerminalScreen::EMPTY_CHAR_VIEW;

void TerminalScreen::move_cursor_to(int line, int column) {
  terminal << "\x1b["sv << line << ';' << column << 'H';
}

void TerminalScreen::move_cursor_by(int lines, int columns) {
  if (lines > 0) {
    terminal << "\x1b["sv << lines << 'B';
  } else if (lines < 0) {
    terminal << "\x1b["sv << -lines << 'A';
  }

  if (columns > 0) {
    terminal << "\x1b["sv << columns << 'C';
  } else if (columns < 0) {
    terminal << "\x1b["sv << -columns << 'D';
  }
}

void TerminalScreen::run_event_loop() {
  event_dispatching_thread_id = std::this_thread::get_id();

  while (not this->quit) {
    terminal.read_events();
    if (auto event = this->event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      dispatch_event(*event);
    }
  }
}

void TerminalScreen::resize_view() {
  auto size = this->size;
  this->size = terminal.get_size();
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

void TerminalScreen::print() {
  move_cursor_to(1, 1);

  const auto *prev_cv = &EMPTY_CHAR_VIEW;

  auto escape_attrs_and_colors = [&](const CharView &cv) {
    auto reset = prev_cv->attributes & ~cv.attributes;
    auto set = ~prev_cv->attributes & cv.attributes;

    escape_attrs(reset, set);

    if (prev_cv->background_color != cv.background_color) {
      escape_background_color(cv.background_color);
    }

    if (prev_cv->foreground_color != cv.foreground_color) {
      escape_foreground_color(cv.foreground_color);
    }

    prev_cv = &cv;
  };

  for (auto y = 0U; y < this->view.size(); ++y) {
    if (y) {
//      escape_attrs_and_colors(EMPTY_CHAR_VIEW);
      terminal << "\r\n"sv;
    }

    auto skip = false;
    for (auto &&cv : this->view[y]) {
      if (not skip) {
        escape_attrs_and_colors(cv);
        terminal << cv.ch;
      }
      skip = cv.is_wide();
    }
  }

  escape_attrs_and_colors(EMPTY_CHAR_VIEW);
}

void TerminalScreen::clear() {
  for (auto &line : this->view) {
    for (auto &cell : line) {
      cell = EMPTY_CHAR_VIEW;
    }
  }
}

void TerminalScreen::flush() {
  print();
  terminal.flush();
}

}
