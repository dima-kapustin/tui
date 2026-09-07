#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>
#include <tui++/terminal/text/TextGraphics.h>

#include <tui++/Window.h>
#include <tui++/lookandfeel/text/TextLookAndFeel.h>
#include <tui++/TextMetrics.h>

#include <tui++/Font.h>

#include <tui++/util/utf-8.h>
#include <tui++/util/log.h>

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

    // Fire due timers (caret blink, ...) on the dispatch thread, ordered
    // after the events they were scheduled between.
    run_pending_timers();
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
    log_resize_ln("text screen " << size.width << 'x' << size.height << " -> " << this->size.width << 'x' << this->size.height);
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
  // screen instead of leaving stale, mis-sized frames behind. Popup windows
  // keep their own size and position: stretching one to the screen size would
  // paint the open menu (and its selection highlight) across the whole
  // window, covering the frame beneath it.
  {
    std::unique_lock lock(this->windows_mutex);
    log_resize_ln("screen resize: " << this->windows.size() << " top-level window(s) to " << this->size.width << 'x' << this->size.height);
    for (auto &&window : this->windows) {
      if (window->get_type() != WindowType::POPUP) {
        window->set_size(this->size);
      }
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

bool TextScreen::row_equals(std::vector<CharView> const &a, std::vector<CharView> const &b, int width) {
  for (auto x = 0; x < width; ++x) {
    if (not same_cell(a[x], b[x])) {
      return false;
    }
  }
  return true;
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

void TextScreen::emit_row(int y, int first_column, int last_column) {
  auto &row = this->view[y];
  auto width = int(row.size());
  if (width <= 0 or last_column <= first_column) {
    return;
  }

  auto &shadow_row = this->shadow[y];
  auto x = first_column;
  while (x < last_column) {
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
}

bool TextScreen::flush_rows_by_terminal_scroll(int first_row, int last_row) {
  auto height = int(this->view.size());
  auto band = last_row - first_row;
  if (band < 2) {
    // A scroll moves at least one row, and needs one more to move it into.
    return false;
  }
  auto width = int(this->view[first_row].size());
  if (width <= 0) {
    return false;
  }

  // The terminal content of every row of the band must be authoritative
  // (fully emitted), and the rows uniform; otherwise the shadow cannot say
  // what a terminal-side scroll would move.
  for (auto y = first_row; y < last_row; ++y) {
    if (not this->row_sent[y]) {
      return false;
    }
    if (int(this->view[y].size()) != width or int(this->shadow[y].size()) != width) {
      return false;
    }
  }

  // A band the view did not change at all emits nothing in the per-run loop;
  // scrolling it would churn the terminal for nothing (and blank rows would
  // match any shift), so leave it to that loop.
  auto changed = false;
  for (auto y = first_row; y < last_row and not changed; ++y) {
    if (not row_equals(this->view[y], this->shadow[y], width)) {
      changed = true;
    }
  }
  if (not changed) {
    return false;
  }

  // The shift of a scroll: the band's content moved up by k (what the
  // terminal showed at row y + k is wanted at row y; the band's bottom k
  // rows hold content that entered it) or down by k (new rows at the top).
  // The smallest k that explains the band is the scroll that produced it,
  // so the shifts are scanned from 1 up. Up to MAX_SCROLL_EXCEPTIONS rows
  // may disagree with a pure shift -- the caret crossing a band edge, the
  // scrollbar thumb edges inside a merged region, a static status row --
  // and are re-emitted afterwards against the scrolled shadow, so the bound
  // only decides how often the optimization applies, never its correctness.
  constexpr int MAX_SCROLL_EXCEPTIONS = 10;

  // Rows of the shift range that are not the shifted shadow; filled by the
  // probe that matched, then copied from the shadow before it is overwritten
  // and re-emitted against it.
  auto exceptions = std::vector<int> { };
  exceptions.reserve(std::size_t(MAX_SCROLL_EXCEPTIONS + 1));

  // Content moved up: view[y] == shadow[y + k] for y in [first_row, last_row - k).
  // A candidate is accepted only when most of its range rows really match
  // (mismatches are a minority): without that floor, a k whose whole range is
  // shorter than the exception budget would be "accepted" with every row
  // mismatching -- e.g. k near the band size -- and scroll the terminal by a
  // shift the content never made.
  auto shift_up = [&] {
    for (auto k = 1; k < band; ++k) {
      exceptions.clear();
      auto range = last_row - k - first_row;
      for (auto y = first_row; y < last_row - k; ++y) {
        if (not row_equals(this->view[y], this->shadow[y + k], width)) {
          exceptions.push_back(y);
          if (int(exceptions.size()) > MAX_SCROLL_EXCEPTIONS) {
            break;
          }
        }
      }
      if (int(exceptions.size()) <= MAX_SCROLL_EXCEPTIONS and int(exceptions.size()) * 2 < range) {
        return k;
      }
    }
    return 0;
  };

  // Content moved down: view[y] == shadow[y - k] for y in [first_row + k, last_row).
  auto shift_down = [&] {
    for (auto k = 1; k < band; ++k) {
      exceptions.clear();
      auto range = last_row - k - first_row;
      for (auto y = first_row + k; y < last_row; ++y) {
        if (not row_equals(this->view[y], this->shadow[y - k], width)) {
          exceptions.push_back(y);
          if (int(exceptions.size()) > MAX_SCROLL_EXCEPTIONS) {
            break;
          }
        }
      }
      if (int(exceptions.size()) <= MAX_SCROLL_EXCEPTIONS and int(exceptions.size()) * 2 < range) {
        return k;
      }
    }
    return 0;
  };

  auto k = shift_up();
  auto content_moved_up = k > 0; // what entered the band: its bottom k rows
  if (k == 0) {
    k = shift_down();
    if (k == 0) {
      return false; // not a scroll: leave the band to the per-run loop
    }
  }

  // The terminal operation moves the band's content the other way:
  //
  //  content moved up by k (a wheel scroll): delete k lines at the band top,
  //    the terminal shifts the band's content up and blanks its bottom k rows;
  //  content moved down by k: insert k lines at the band top, the terminal
  //    shifts the band's content down and blanks its top k rows.
  // The blanked edge is where the content that entered the band is painted.
  // When the band is the whole screen no scroll region is needed (and none is
  // left behind); otherwise the band is bracketed by a scroll region that is
  // reset to the full screen right after the operation.
  log_graphics_ln("terminal scroll: band " << first_row << ".." << last_row - 1 << (content_moved_up ? " up " : " down ") << k << " row(s), " << exceptions.size() << " row(s) deviate");

  auto full_screen_band = first_row == 0 and last_row == height;
  if (not full_screen_band) {
    terminal << "\x1b["sv << first_row + 1 << ';' << last_row << 'r';
  }
  // The delete/insert applies at the band's top row (the API is 1-based).
  move_cursor_to(first_row + 1, 1);
  terminal << "\x1b["sv << k << (content_moved_up ? 'M' : 'L');
  if (not full_screen_band) {
    terminal << "\x1b[r"sv;
  }

  // The shadow of every shifted row must become the content the terminal now
  // shows there: the exception rows keep the *shifted* shadow (their screen
  // content moved with the scroll, so they are re-emitted against it below),
  // and the rows a pure shift reproduced equal the view by the match above.
  // The exceptions are copied from the shadow in an order that never reads a
  // row a pass already overwrote: up-shifts read the row above the exception
  // (scan upward), down-shifts read the row below it (scan downward).
  auto new_row_first = first_row;
  auto new_row_last = first_row + k; // top edge: the rows the insert blanked
  if (content_moved_up) {
    for (auto y = first_row; y < last_row - k; ++y) {
      if (std::find(exceptions.begin(), exceptions.end(), y) != exceptions.end()) {
        std::copy(this->shadow[y + k].begin(), this->shadow[y + k].end(), this->shadow[y].begin());
      } else {
        std::copy(this->view[y].begin(), this->view[y].end(), this->shadow[y].begin());
      }
    }
    new_row_first = last_row - k;
    new_row_last = last_row; // bottom edge: the rows the delete blanked
  } else {
    for (auto y = last_row - 1; y >= first_row + k; --y) {
      if (std::find(exceptions.begin(), exceptions.end(), y) != exceptions.end()) {
        std::copy(this->shadow[y - k].begin(), this->shadow[y - k].end(), this->shadow[y].begin());
      } else {
        std::copy(this->view[y].begin(), this->view[y].end(), this->shadow[y].begin());
      }
    }
  }

  // The exception rows disagree with the shift: emit their difference against
  // the scrolled shadow (the terminal's real content), a few cells each for a
  // caret or a scrollbar thumb edge.
  for (auto y : exceptions) {
    emit_row(y, 0, width);
  }

  // The rows the terminal blanked hold content that entered the band; their
  // whole width must be emitted (the blank fill is not the shadow), wide
  // glyphs handled like the run emitter handles them.
  for (auto y = new_row_first; y < new_row_last; ++y) {
    auto &row = this->view[y];
    auto &shadow_row = this->shadow[y];
    move_cursor_to(int(y) + 1, 1);
    auto skip = false;
    for (auto x = 0; x < width; ++x) {
      auto const &cv = row[x];
      if (not skip) {
        escape_to(cv);
        terminal << cv.ch;
      }
      skip = this->text_metrics->get_char_width(cv.ch.get_code()) == 2;
    }
    std::copy(row.begin(), row.end(), shadow_row.begin());
    this->emitted_any = true;
  }

  return true;
}

void TextScreen::flush_rows(Rectangle const &region) {
  auto height = int(this->view.size());
  auto first_row = region.y < 0 ? 0 : region.y;
  auto last_row = region.y + region.height > height ? height : region.y + region.height;
  if (first_row >= last_row) {
    return;
  }

  // When the damaged band is the terminal content shifted by a whole number
  // of rows (a scroll), the terminal scrolls its own buffer and only the rows
  // that entered the band are emitted (see flush_rows_by_terminal_scroll); a
  // wheel notch then costs a few rows instead of the whole band. Any other
  // damage falls through to the per-row emission below.
  if (flush_rows_by_terminal_scroll(first_row, last_row)) {
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

    emit_row(y, first, last);

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
