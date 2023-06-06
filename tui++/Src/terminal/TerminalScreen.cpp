#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalScreen.h>
#include <tui++/terminal/TerminalGraphics.h>

#include <tui++/util/utf-8.h>

#include <iostream>

namespace tui::terminal {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

void escape_attrs(std::ostream &os, const Attributes &reset, const Attributes &set) {
  os << "\x1B[";

  auto pos = os.tellp();
  for (auto &&attr : reset) {
    if (pos != os.tellp()) {
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

  for (auto &&attr : set) {
    if (pos != os.tellp()) {
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

  os << 'm';
}

void escape_foreground_color(std::ostream &os, const Color &color) {
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

void escape_background_color(std::ostream &os, const Color &color) {
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

void TerminalScreen::run_event_loop() {
  while (not this->quit) {
    this->terminal.read_events();
    if (auto event = this->event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      std::cout << *event << std::endl;
    }
  }
}

void TerminalScreen::refresh() {
  auto size = this->size;
  this->size = this->terminal.get_size();
  if (size != this->size) {
    this->chars.resize(this->size.height);
    for (auto &&row : this->chars) {
      row.resize(this->size.width);
    }
  }

  auto g = TerminalGraphics { *this };
  paint(g);
  flush();
}

std::string TerminalScreen::to_string() const {
  std::string buffer;
  buffer.reserve(4 * get_width() * get_height() + get_height());
  std::ostringstream os(buffer);

  const auto default_cv = CharView { { ' ' } };
  const auto *prev_cv = &default_cv;

  auto apply_attrs_and_colors = [&](const CharView &cv) {
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

  for (auto y = 0; y < get_height(); ++y) {
    if (y) {
      apply_attrs_and_colors(default_cv);
      os << "\r\n";
    }

    auto skip = false;
    for (auto &&cv : this->chars[y]) {
      if (not skip) {
        apply_attrs_and_colors(cv);
        os << cv.ch;
      }
      skip = cv.is_wide();
    }
  }

  apply_attrs_and_colors(default_cv);

  return os.str();
}

void TerminalScreen::flush() const {
  std::cout << to_string();
}

}
