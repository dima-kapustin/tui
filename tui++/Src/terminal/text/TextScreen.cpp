#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>
#include <tui++/terminal/text/TextGraphics.h>

#include <tui++/Window.h>
#include <tui++/lookandfeel/text/TextLookAndFeel.h>
#include <tui++/TextMetrics.h>

#include <tui++/Font.h>

#include <tui++/util/utf-8.h>

#include <string_view>

using namespace std::string_view_literals;

namespace tui {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

// The most events dispatched per event-loop iteration. Mouse motion is read
// in bursts and an event may post more events while it is dispatched, so the
// batch is capped to keep a burst (or a self-reposting timer) from starving
// the loop; the iteration then repaints once, covering the whole batch.
constexpr int MAX_EVENTS_PER_TICK = 64;

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

constexpr std::array<std::string_view, 33> palette16_color_codes = //
    { "30", "40",   //
      "31", "41",   //
      "32", "42",   //
      "33", "43",   //
      "34", "44",   //
      "35", "45",   //
      "36", "46",   //
      "37", "47",   //
      "90", "100",  //
      "91", "101",  //
      "92", "102",  //
      "93", "103",  //
      "94", "104",  //
      "95", "105",  //
      "96", "106",  //
      "97", "107" };

static void escape_background_color(TextColor const &color) {
  struct SetBackgroundColor {
    void operator()(detail::DefaultColor const&) {
      terminal << "\x1b[49m"sv;
    }

    void operator()(detail::Palette16Color const &c) {
      terminal << "\x1b["sv << palette16_color_codes[2 * c.index + 1] << 'm';
    }

    void operator()(detail::Palette256Color const &c) {
      terminal << "\x1b[48;5;"sv << c.index << 'm';
    }

    void operator()(detail::TrueColor const &c) {
      terminal << "\x1b[48;2;"sv << c.red << ';' << c.green << ';' << c.blue << 'm';
    }
  };
  std::visit(SetBackgroundColor { }, color);
}

static void escape_foreground_color(TextColor const &color) {
  struct SetForegroundColor {
    void operator()(detail::DefaultColor const&) {
      terminal << "\x1b[39m"sv;
    }

    void operator()(detail::Palette16Color const &c) {
      terminal << "\x1b["sv << palette16_color_codes[2 * c.index] << 'm';
    }

    void operator()(detail::Palette256Color const &c) {
      terminal << "\x1b[38;5;"sv << c.index << 'm';
    }

    void operator()(detail::TrueColor const &c) {
      terminal << "\x1b[38;2;"sv << c.red << ';' << c.green << ';' << c.blue << 'm';
    }
  };
  std::visit(SetForegroundColor { }, color);
}

TextScreen::CharView TextScreen::EMPTY_CHAR_VIEW;

TextScreen::TextScreen() noexcept {
  this->look_and_feel = std::make_shared<laf::TextLookAndFeel>();
  this->text_metrics = std::make_shared<CellTextMetrics>(Font { });
  resize_view();
}

void TextScreen::move_cursor_to(int line, int column) {
  terminal << "\x1b["sv << line << ';' << column << 'H';
}

void TextScreen::move_cursor_by(int lines, int columns) {
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

void TextScreen::run_event_loop() {
  event_dispatching_thread_id = std::this_thread::get_id();

  while (not this->quit) {
    terminal.read_events();

    // Dispatch a bounded batch of events. Repainting is not a side effect of
    // the loop: repaint() requests accumulate damaged regions and schedule a
    // single repaint invocation on the same queue (see Screen::add_damage),
    // so the paint is dispatched here in order with the mouse/key events that
    // caused it -- Swing's RepaintManager behaves the same way. The batch cap
    // keeps a burst (or a self-reposting timer) from starving the loop.
    auto event = this->event_queue.pop(WAIT_EVENT_TIMEOUT);
    for (auto n = 0; event and n < MAX_EVENTS_PER_TICK; ++n) {
      dispatch_event(*event);
      event = this->event_queue.pop(std::chrono::milliseconds::zero());
    }
  }
}

std::unique_ptr<Graphics> TextScreen::get_graphics() {
  return std::make_unique<TextGraphics>(*this);
}

std::unique_ptr<Graphics> TextScreen::get_graphics(Rectangle const &clip) {
  return std::make_unique<TextGraphics>(*this, Rectangle { 0, 0, clip.width, clip.height }, clip.x, clip.y);
}

void TextScreen::resize_view() {
  auto size = this->size;
  this->size = terminal.get_size();
  if (size != this->size) {
    this->view.resize(this->size.height);
    for (auto &&row : this->view) {
      row.resize(this->size.width);
      std::fill(row.begin(), row.end(), EMPTY_CHAR_VIEW);
    }
  }
}

void TextScreen::resized() {
  resize_view();
  // Top-level windows track the terminal size so the layout fills the new
  // screen instead of leaving stale, mis-sized frames behind.
  {
    std::unique_lock lock(this->windows_mutex);
    for (auto &&window : this->windows) {
      window->set_size(this->size);
    }
  }
  refresh();
}

void TextScreen::refresh() {
  // A full repaint covers every pending damaged region, so drop them; any
  // queued repaint invocation then becomes a no-op instead of repainting the
  // whole screen a second time.
  this->damaged_regions.clear();

  auto g = TextGraphics { *this };
  paint(g);
  flush();
}

void TextScreen::repaint_region(Rectangle const &rect) {
  auto region = rect & Rectangle { 0, 0, get_width(), get_height() };
  if (region.empty()) {
    return;
  }

  // Paint the tree with a graphics clipped to the region: every draw is
  // clipped to it, so only the damaged cells of the view change. The view
  // itself is the back buffer, so emitting only the rows the damage touches
  // is enough -- the untouched rows still hold their last painted content.
  auto g = TextGraphics { *this, region, 0, 0 };
  paint(g);

  print_rows_region(region);
  terminal.flush();
}

TextColor TextScreen::to_terminal(Color const &c) {
  return detail::TrueColor { c.red(), c.green(), c.blue() };
}

void TextScreen::print() {
  print_rows(0, int(this->view.size()));
}

void TextScreen::print_rows(int first_row, int last_row) {
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

  for (auto y = first_row; y < last_row; ++y) {
    // Position each row absolutely: a line feed after the last row would
    // scroll the screen when the cursor sits on the bottom row, shifting
    // everything the next screen draws off by a row.
    move_cursor_to(int(y) + 1, 1);

    auto skip = false;
    for (auto &&cv : this->view[y]) {
      if (not skip) {
        escape_attrs_and_colors(cv);
        terminal << cv.ch;
      }
      skip = this->text_metrics->get_char_width(cv.ch.get_code()) == 2;
    }
  }

  escape_attrs_and_colors(EMPTY_CHAR_VIEW);
}

void TextScreen::print_rows_region(Rectangle const &region) {
  const auto *prev_cv = &EMPTY_CHAR_VIEW;

  // Same delta emission as print_rows: only the attributes/colors that change
  // between two neighbouring cells are emitted, so an unchanged run inside a
  // damaged row costs plain characters, not control sequences.
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

  auto height = int(this->view.size());
  auto first_row = region.y < 0 ? 0 : region.y;
  auto last_row = region.y + region.height > height ? height : region.y + region.height;
  if (first_row >= last_row) {
    return;
  }

  for (auto y = first_row; y < last_row; ++y) {
    auto const &row = this->view[y];
    auto width = int(row.size());
    if (width <= 0) {
      continue;
    }

    // Whole row vs damaged-span emission: a damaged region that covers most
    // of a row is cheapest as one absolute cursor move plus a contiguous
    // write of the whole row (the back buffer already holds every cell). A
    // narrow slice is cheaper as one cursor move to its head plus only the
    // slice's cells, with the full state emitted up front. Every run is
    // self-describing: it is positioned absolutely and starts from the empty
    // state, so neighbouring content (rows outside the region, cells beside
    // the slice) is left untouched on the terminal.
    auto whole_row = region.width * 2 >= width;
    auto first = whole_row ? 0 : std::max(0, region.x);
    auto last = whole_row ? width : std::min(width, region.x + region.width);
    if (last <= first) {
      continue;
    }

    // A damaged slice can start on the continuation cell of a double-width
    // glyph whose leading cell sits just outside the slice. That cell holds
    // no glyph of its own (the row emitter skips it), so back the slice up by
    // one and let the leading cell print its full-width glyph; otherwise the
    // terminal would show the placeholder as a character of its own.
    if (first > 0 and this->text_metrics->get_char_width(row[first - 1].ch.get_code()) == 2) {
      --first;
    }

    move_cursor_to(int(y) + 1, first + 1);
    prev_cv = &EMPTY_CHAR_VIEW;
    auto skip = false;
    for (auto x = first; x < last; ++x) {
      auto const &cv = row[x];
      if (not skip) {
        escape_attrs_and_colors(cv);
        terminal << cv.ch;
      }
      skip = this->text_metrics->get_char_width(cv.ch.get_code()) == 2;
    }
  }

  escape_attrs_and_colors(EMPTY_CHAR_VIEW);
}

void TextScreen::clear() {
  for (auto &line : this->view) {
    for (auto &cell : line) {
      cell = EMPTY_CHAR_VIEW;
    }
  }
}

void TextScreen::flush() {
  print();
  terminal.flush();
}

}
