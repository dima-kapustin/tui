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
  this->last_state = EMPTY_CHAR_VIEW;
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
    this->shadow.resize(this->size.height);
    this->row_sent.resize(this->size.height);
    for (auto y = 0; y < this->size.height; ++y) {
      this->view[y].resize(this->size.width);
      this->shadow[y].resize(this->size.width);
      std::fill(this->view[y].begin(), this->view[y].end(), EMPTY_CHAR_VIEW);
      std::fill(this->shadow[y].begin(), this->shadow[y].end(), EMPTY_CHAR_VIEW);
      // The terminal still shows the pre-resize content, so nothing is
      // "sent" yet: the next flush re-emits every row.
      this->row_sent[y] = false;
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

  // The terminal content may be unknown (first paint, resize), so treat this
  // as a fresh pass: rows not marked as sent are emitted in full.
  this->emitted_any = false;
  auto g = TextGraphics { *this };
  paint(g);
  flush_rows(Rectangle { 0, 0, get_width(), get_height() });
  end_flush();
}

void TextScreen::repaint_region(Rectangle const &rect) {
  auto region = rect & Rectangle { 0, 0, get_width(), get_height() };
  if (region.empty()) {
    return;
  }

  // Paint the tree with a graphics clipped to the region: every draw is
  // clipped to it, so only the damaged cells of the view change. The view
  // itself is the back buffer, so emitting only what changed in the rows the
  // damage touches is enough -- the untouched rows still hold their last
  // painted content.
  auto g = TextGraphics { *this, region, 0, 0 };
  paint(g);

  flush_rows(region);
  // No terminal flush here: the repaint pass (Screen::repaint_damaged)
  // flushes once after all regions, and only when something was emitted.
}

void TextScreen::repaint_pass_begin() {
  this->emitted_any = false;
}

void TextScreen::repaint_pass_end() {
  end_flush();
}

TextColor TextScreen::to_terminal(Color const &c) {
  return detail::TrueColor { c.red(), c.green(), c.blue() };
}

bool TextScreen::same_cell(CharView const &a, CharView const &b) {
  return a.ch.get_code() == b.ch.get_code() and a.attributes == b.attributes and a.foreground_color == b.foreground_color and a.background_color == b.background_color;
}

void TextScreen::escape_to(CharView const &cv) {
  auto reset = this->last_state.attributes & ~cv.attributes;
  auto set = ~this->last_state.attributes & cv.attributes;

  escape_attrs(reset, set);

  if (this->last_state.background_color != cv.background_color) {
    escape_background_color(cv.background_color);
  }

  if (this->last_state.foreground_color != cv.foreground_color) {
    escape_foreground_color(cv.foreground_color);
  }

  this->last_state = cv;
}

void TextScreen::flush_rows(Rectangle const &region) {
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

    // Compare (and if needed emit) the damaged span; a row never sent before
    // is compared in full, whatever the region says.
    auto sent = this->row_sent[y];
    auto first = sent ? std::max(0, region.x) : 0;
    auto last = sent ? std::min(width, region.x + region.width) : width;
    if (last <= first) {
      continue;
    }
    auto full_row_scan = first == 0 and last == width;

    auto &shadow_row = this->shadow[y];
    auto x = first;
    while (x < last) {
      if (same_cell(row[x], shadow_row[x])) {
        ++x;
        continue;
      }

      // A changed run starts at x. If it starts on the continuation cell of
      // a double-width glyph whose leading cell sits just outside the run,
      // back the run up by one: that cell holds no glyph of its own (the
      // emitter skips it after its wide neighbour), so printing the run as
      // is would show the placeholder as a character of its own.
      auto run_first = x;
      if (run_first > 0 and this->text_metrics->get_char_width(row[run_first - 1].ch.get_code()) == 2) {
        --run_first;
      }

      // Extend while cells differ; the cells after the run equal the shadow
      // and are already on the terminal.
      auto run_last = run_first + 1;
      while (run_last < width and not same_cell(row[run_last], shadow_row[run_last])) {
        ++run_last;
      }
      // Do not end the run on the continuation cell of a wide glyph: extend
      // past it so its (skipped) cell stays inside the run's bookkeeping.
      while (run_last < width and run_last > run_first and this->text_metrics->get_char_width(row[run_last - 1].ch.get_code()) == 2) {
        ++run_last;
      }

      // The run is positioned absolutely and starts from the SGR state the
      // terminal is in (the previous emission, possibly in an earlier row or
      // pass), so neighbouring content is left untouched.
      move_cursor_to(int(y) + 1, run_first + 1);
      auto skip = false;
      for (auto i = run_first; i < run_last; ++i) {
        auto const &cv = row[i];
        if (not skip) {
          escape_to(cv);
          terminal << cv.ch;
        }
        skip = this->text_metrics->get_char_width(cv.ch.get_code()) == 2;
      }

      // The terminal now shows the run's cells; record them as sent.
      std::copy(row.begin() + run_first, row.begin() + run_last, shadow_row.begin() + run_first);
      this->emitted_any = true;
      x = run_last;
    }

    if (full_row_scan) {
      this->row_sent[y] = true;
    }
  }
}

void TextScreen::end_flush() {
  if (this->emitted_any) {
    // Leave the terminal with default attributes, so whatever is printed
    // after the app (or the next flush, which starts from the empty state)
    // is not styled by our last cell.
    escape_to(EMPTY_CHAR_VIEW);
    terminal.flush();
    this->emitted_any = false;
  }
}

void TextScreen::clear() {
  // The terminal still shows whatever was there, so nothing counts as sent:
  // the next flush re-emits every row.
  this->emitted_any = false;
  for (auto y = 0; y < int(this->view.size()); ++y) {
    std::fill(this->view[y].begin(), this->view[y].end(), EMPTY_CHAR_VIEW);
    std::fill(this->shadow[y].begin(), this->shadow[y].end(), EMPTY_CHAR_VIEW);
    this->row_sent[y] = false;
  }
}

void TextScreen::flush() {
  // Full-screen flush of the view's current content (the direct-paint path,
  // e.g. TextGraphics::flush). Idempotent with the shadow: only the rows
  // that actually changed since the last flush reach the terminal.
  this->emitted_any = false;
  flush_rows(Rectangle { 0, 0, get_width(), get_height() });
  end_flush();
}

}
