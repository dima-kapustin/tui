#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>
#include <tui++/terminal/text/TextGraphics.h>

#include <tui++/Window.h>
#include <tui++/lookandfeel/text/TextLookAndFeel.h>
#include <tui++/TextMetrics.h>

#include <tui++/Font.h>

#include <tui++/util/utf-8.h>
#include <tui++/util/log.h>

#include <cmath>
#include <string_view>

using namespace std::string_view_literals;

namespace tui {

namespace {

// The squared RGB distance between two cell colors, with the empty
// (terminal-default) color treated as black. Used to judge how visible the
// terminal's blanked-edge background is against the content that will
// replace it (see flush_rows_by_terminal_scroll).
long long bg_distance_sq(TextColor const &a, TextColor const &b) {
  auto rgb = [](TextColor const &c) {
    return std::visit([](auto const &v) {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, std::monostate>) {
        return detail::TrueColor { 0, 0, 0 };
      } else {
        return detail::TrueColor { v };
      }
    }, c);
  };
  auto ca = rgb(a);
  auto cb = rgb(b);
  auto dr = int(ca.red) - int(cb.red);
  auto dg = int(ca.green) - int(cb.green);
  auto db = int(ca.blue) - int(cb.blue);
  return 1LL * dr * dr + 1LL * dg * dg + 1LL * db * db;
}

// A cell color shifted towards `overlay` by `opacity` (0..1): the shade the
// cells under a shadow take (see TextScreen::blend_rect). The cell's own
// color is kept -- only moved towards the shadow's -- so the content stays
// readable; the result is a truecolor whatever the base was, since the blend
// lands between palette entries. A color the program never set is the
// terminal's own default, which `terminal_default` holds when the terminal
// said what it is (see TextScreen::set_default_colors); without an answer
// there is nothing to blend against and the cell is left alone.
TextColor shade(TextColor const &base, Color const &overlay, double opacity, std::optional<Color> const &terminal_default) {
  auto rgb = std::visit([&terminal_default](auto const &value) -> std::optional<detail::TrueColor> {
    using T = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<T, detail::DefaultColor>) {
      if (terminal_default) {
        return detail::TrueColor { terminal_default->red(), terminal_default->green(), terminal_default->blue() };
      }
      return std::nullopt;
    } else {
      return static_cast<detail::TrueColor>(value);
    }
  }, base);

  if (not rgb) {
    return base;
  }

  auto mix = [opacity](uint8_t from, uint8_t to) {
    return uint8_t(std::lround(from * (1 - opacity) + to * opacity));
  };
  return detail::TrueColor { mix(rgb->red, overlay.red()), mix(rgb->green, overlay.green()), mix(rgb->blue, overlay.blue()) };
}

}

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

  // Ask the terminal for its own colors before the first frame: the shadows
  // blend the cells a program never painted against them (see blend_rect),
  // and the round-trip would otherwise swallow a keystroke when the first
  // popup opens. Terminals that do not answer leave the colors unknown.
  if (auto colors = terminal.query_default_colors(); colors.foreground or colors.background) {
    set_default_colors(colors.foreground, colors.background);
  }

  // Coalesce repaint requests onto a frame clock: input bursts (fast mouse
  // motion) damage repeatedly between paints and each paint costs a terminal
  // write, so painting at the next frame boundary after the first damage
  // renders the latest state at most once per interval -- the highlight then
  // trails the cursor by one frame instead of by the backlog of the input
  // burst.
  this->repaint_interval = std::chrono::milliseconds { 16 };

  while (not this->quit) {
    terminal.read_events();

    // Dispatch a bounded batch of events. Repainting is not a side effect of
    // the loop: repaint() requests accumulate damaged regions and schedule a
    // single repaint (on the queue, or on the frame clock -- see
    // Screen::add_damage). The batch cap keeps a burst (or a self-reposting
    // timer) from starving the loop. The wait is shortened to the pending
    // repaint's due time, so an idle screen still paints at its frame
    // boundary.
    auto wait = WAIT_EVENT_TIMEOUT;
    auto now = std::chrono::steady_clock::now();
    if (this->repaint_event_pending and this->repaint_due <= now + wait) {
      wait = std::chrono::duration_cast<std::chrono::milliseconds>(this->repaint_due - now);
      wait = std::max(wait, std::chrono::milliseconds::zero());
    }
    auto event = this->event_queue.pop(wait);
    for (auto n = 0; event and n < MAX_EVENTS_PER_TICK; ++n) {
      dispatch_event(*event);
      event = this->event_queue.pop(std::chrono::milliseconds::zero());
    }

    // Fire due timers (caret blink, ...) on the dispatch thread, ordered
    // after the events they were scheduled between.
    run_pending_timers();

    // Paint the frame once its boundary is due: every request that arrived
    // since the boundary was armed is painted together.
    repaint_if_due();
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

  // A repaint starts from a clean slate: cells the tree no longer paints must
  // go back to "nothing" (the terminal's default background). Whatever is
  // left in the view -- a widget that moved, a layout that changed, a closed
  // window -- would otherwise stay on screen, because the flush only emits
  // what differs from the view.
  for (auto &&row : this->view) {
    std::fill(row.begin(), row.end(), EMPTY_CHAR_VIEW);
  }

  // The terminal content may be unknown (first paint, resize), so treat this
  // as a fresh pass: rows not marked as sent are emitted in full.
  this->emitted_any = false;
  auto g = TextGraphics { *this };
  paint(g);
  flush_rows(Rectangle { 0, 0, get_width(), get_height() });
  end_flush();
}

void TextScreen::on_window_removed(Rectangle const &area) {
  // The removed window's cells were painted over the windows beneath it, so
  // after it is gone the repaint of those windows only overwrites the cells
  // they draw; anything else would keep the window's content in the view
  // forever. Reset the area to the empty (never painted) state before that
  // repaint runs -- the flush then emits the cells the repaint leaves empty
  // as erasures, which is exactly what painting the remaining windows over a
  // clean buffer would show.
  auto rect = area & Rectangle { 0, 0, get_width(), get_height() };
  if (rect.empty()) {
    return;
  }
  for (auto y = rect.y; y < rect.bottom(); ++y) {
    auto &row = this->view[y];
    std::fill(row.begin() + rect.x, row.begin() + rect.right(), EMPTY_CHAR_VIEW);
  }
}

void TextScreen::repaint_region(Rectangle const &rect) {
  auto region = rect & Rectangle { 0, 0, get_width(), get_height() };
  if (region.empty()) {
    return;
  }

  // The damaged region is repainted from scratch: reset its cells first, so a
  // component that moved or shrank does not leave its old content in the
  // cells it no longer paints (the cells then read as "nothing" and the
  // flush emits them as erasures wherever the shadow still holds the old
  // content). Cells the repaint does paint over cost nothing extra: the
  // flush still diffs the result against the shadow.
  for (auto y = region.y; y < region.bottom(); ++y) {
    auto &&row = this->view[y];
    std::fill(row.begin() + region.x, row.begin() + region.right(), EMPTY_CHAR_VIEW);
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

void TextScreen::blend_rect(Rectangle const &rect, Color const &color, double opacity) {
  auto region = rect & Rectangle { 0, 0, get_width(), get_height() };
  if (region.empty() or opacity <= 0) {
    return;
  }
  opacity = std::min(opacity, 1.0);

  // Only the cells' colors change: the shadow does not write glyphs, and the
  // flush emits exactly the cells whose color moved (the view against the
  // terminal shadow), so an unchanged cell under a shadow still costs
  // nothing.
  for (auto y = region.y; y < region.bottom(); ++y) {
    auto &&row = this->view[y];
    for (auto x = region.x; x < region.right(); ++x) {
      auto &&cell = row[x];
      cell.foreground_color = shade(cell.foreground_color, color, opacity, this->default_foreground);
      cell.background_color = shade(cell.background_color, color, opacity, this->default_background);
    }
  }
}

TextColor TextScreen::to_terminal(Color const &c) {
  return detail::TrueColor { c.red(), c.green(), c.blue() };
}

bool TextScreen::same_cell(CharView const &a, CharView const &b) {
  if (a.ch.get_code() != b.ch.get_code() or a.attributes != b.attributes or a.background_color != b.background_color) {
    return false;
  }
  // A plain blank cell (space, no attributes) shows only its background: the
  // terminal renders nothing of its foreground. Ignoring the foreground there
  // keeps a width growth from repainting the newly exposed column cell-by-cell
  // (the view paints its own fill colors, the viewport's blank fill left the
  // default ones -- on a blank cell that difference is invisible). With an
  // attribute the foreground can show (inverse turns a space into a block,
  // underline draws a line), so it is compared then.
  if (a.ch.get_code() != ' ' or a.attributes != Attributes::NONE) {
    if (a.foreground_color != b.foreground_color) {
      return false;
    }
  }
  return true;
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

void TextScreen::emit_whole_row(int y, int first, int last) {
  auto &row = this->view[y];
  auto &shadow_row = this->shadow[y];

  // The whole span was damaged, so there is nothing to gain by skipping the
  // cells that happen to equal the shadow; emit it as one contiguous run.
  move_cursor_to(int(y) + 1, first + 1);
  // A span may start on the continuation cell of a wide glyph whose leading
  // cell sits just before it; that cell holds no glyph of its own, so it must
  // be skipped the way the run emitter skips it.
  auto skip = first > 0 and this->text_metrics->get_char_width(row[first - 1].ch.get_code()) == 2;
  for (auto x = first; x < last; ++x) {
    auto const &cv = row[x];
    if (not skip) {
      escape_to(cv);
      terminal << cv.ch;
    }
    skip = this->text_metrics->get_char_width(cv.ch.get_code()) == 2;
  }
  std::copy(row.begin() + first, row.begin() + last, shadow_row.begin() + first);
  this->emitted_any = true;
}

bool TextScreen::flush_rows_by_terminal_scroll(int first_row, int last_row, int first_column, int last_column) {
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

  // A terminal-side scroll moves the whole band, including any overlapping
  // top-level window (an open popup menu over the scrolled area). Those cells
  // are not part of the scrolling surface: scrolling them would drag the
  // popup with the content and then re-emit it back -- a visible flicker.
  // Refuse the optimization while a popup overlaps the band; the per-run
  // fallback redraws the scrolled rows in place and leaves the popup alone.
  {
    std::unique_lock lock(this->windows_mutex);
    for (auto const &window : this->windows) {
      if (window->get_type() == WindowType::POPUP and window->is_visible()) {
        auto top = window->get_y();
        auto bottom = top + window->get_height();
        if (bottom > first_row and top < last_row) {
          return false;
        }
      }
    }
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

  // A one-row shift is where a terminal-side scroll does the least good and
  // the most visible harm: moving the band also moves a fixed column next to
  // it (a scroll bar's thumb and arrows), which the exception re-emission then
  // draws back -- a "damage then fix" flicker. The per-run loop repaints such
  // a column in place for a one-row scroll at nearly the same cost, so leave
  // k == 1 to it; a two-or-more-row shift is where the terminal scroll wins.
  if (k < 2) {
    return false;
  }

  // The shift must be a real scroll, not a band that happened to absorb a
  // fixed row (a horizontal scroll bar, a status line, a border). A fixed row
  // does not move with the content, so it shows up as an exception whose whole
  // width differs from the shifted shadow; a scroll bar's thumb edge differs
  // in only a few cells. Refuse the scroll when such a full-width fixed row is
  // present: the terminal scroll would drag it with the content and re-emit it
  // back -- the same flicker the damage merge above avoids.
  for (auto y : exceptions) {
    auto const &shifted = content_moved_up ? this->shadow[y + k] : this->shadow[y - k];
    auto differing = 0;
    for (auto x = 0; x < width; ++x) {
      if (not same_cell(this->view[y][x], shifted[x])) {
        ++differing;
      }
    }
    if (differing * 2 > width) {
      return false;
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

  // The blanked edge: the rows the terminal blanks that the content entering
  // the band is painted into. Computed up front so its background can be
  // applied before the scroll (below).
  auto new_row_first = content_moved_up ? last_row - k : first_row;
  auto new_row_last = content_moved_up ? last_row : first_row + k;

  // The terminal paints the revealed edge with ONE solid background (the mask
  // set below). Wherever the final content of the entering rows differs from
  // that background, the terminal shows the mask color for the frame between
  // the scroll and the fill. Judge the difference by its visual weight -- the
  // sum of the squared color distances over the damaged columns -- and refuse
  // the scroll when it would be seen: a highlighted row entering the edge is
  // an obvious flash, while a couple of fixed columns of a slightly different
  // background at the band's edges (the pane showing through next to the
  // content) are imperceptible and stay allowed.
  //
  // The budget is the weight of one fully saturated cell (256^2): smaller than
  // the weight of one highlighted row (~77 cells of a selection or cursor
  // background) or of the two border columns of a full-screen band, and far
  // above the weight of the few near-identical fixed columns next to the
  // content.
  constexpr long long FLASH_BUDGET = 65536;
  auto const &mask = this->view[new_row_first][width > 1 ? 1 : 0];
  auto flash = 0LL;
  for (auto y = new_row_first; y < new_row_last and flash <= FLASH_BUDGET; ++y) {
    for (auto x = first_column; x < last_column; ++x) {
      flash += bg_distance_sq(this->view[y][x].background_color, mask.background_color);
    }
  }
  if (flash > FLASH_BUDGET) {
    log_graphics_ln("edge flash check refused the scroll (weight " << flash << ")");
    return false;
  }

  // The terminal blanks the revealed edge with its current background color.
  // Set it to the entering content's background before scrolling so the blank
  // is invisible and the fill that follows is seamless; otherwise a fast
  // scroll flashes the blanked edge with the default background for the frame
  // between the scroll and the fill. Sample the first CONTENT cell of the
  // entering row (column 1, inside the band): column 0 is the frame border,
  // whose colors (the theme's blue/cyan double line) would paint the whole
  // blanked edge blue.
  escape_to(this->view[new_row_first][width > 1 ? 1 : 0]);

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
  if (content_moved_up) {
    for (auto y = first_row; y < last_row - k; ++y) {
      if (std::find(exceptions.begin(), exceptions.end(), y) != exceptions.end()) {
        std::copy(this->shadow[y + k].begin(), this->shadow[y + k].end(), this->shadow[y].begin());
      } else {
        std::copy(this->view[y].begin(), this->view[y].end(), this->shadow[y].begin());
      }
    }
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
  // of rows (a scroll), flush_rows_by_terminal_scroll can move the terminal's
  // own buffer and emit only the rows that entered the band. That path is
  // disabled: the terminal scroll blanks the revealed edge first and fills it
  // afterwards, and the blank shows for one frame no matter how well it is
  // masked -- a flicker at the top or bottom of the band on every scroll.
  // The per-row emission below repaints every changed cell in place (the path
  // the popup-overlap case has always used): no blank, no intermediate state,
  // no flicker, at the cost of re-emitting the changed band. Flip the switch
  // when the byte cost matters more than the edge transient.
  constexpr bool USE_TERMINAL_SCROLL = false;
  auto first_column = std::max(0, region.x);
  auto last_column = std::min(int(this->view[first_row].size()), region.x + region.width);
  if (USE_TERMINAL_SCROLL and flush_rows_by_terminal_scroll(first_row, last_row, first_column, last_column)) {
    return;
  }

  for (auto y = first_row; y < last_row; ++y) {
    auto const &row = this->view[y];
    auto width = int(row.size());
    if (width <= 0) {
      continue;
    }

    // Compare (and if needed emit) the damaged span; a row never sent before
    // is not compared at all: the terminal's content for it is unknown, so
    // the row is written whole below, whatever the region says.
    auto sent = this->row_sent[y];
    auto first = sent ? std::max(0, region.x) : 0;
    auto last = sent ? std::min(width, region.x + region.width) : width;
    if (last <= first) {
      continue;
    }

    if (not sent) {
      // A row the terminal never received in full -- the first paint, or a
      // resize that left its content unknown -- must be written whole, blanks
      // included. Diffing it against the fresh shadow would only emit the
      // cells the tree paints and leave every other cell showing whatever the
      // terminal had there (the pre-resize layout); writing the whole row
      // clears them to the default background, the way a repaint from scratch
      // would.
      emit_whole_row(y, 0, width);
      this->row_sent[y] = true;
      continue;
    }

    // A dense, wide change (an edit that shifts a line's tail) fragments the
    // per-run shadow diff into many cursor moves -- a visual "snake" -- so
    // emit the changed span as one run. Measure density over the span between
    // the first and last changed cell, not the full damaged row: a short line
    // leaves most of the row empty, which would look sparse if measured
    // against the whole row. A sparse or narrow change (a caret move, a thumb
    // edge) still wins by emitting only its differing runs.
    auto &shadow_row = this->shadow[y];
    auto first_changed = -1;
    auto last_changed = -1;
    auto changed = 0;
    for (auto x = first; x < last; ++x) {
      if (not same_cell(row[x], shadow_row[x])) {
        if (first_changed < 0) {
          first_changed = x;
        }
        last_changed = x;
        ++changed;
      }
    }
    if (first_changed < 0) {
      continue; // nothing changed in this row
    }
    auto changed_span = last_changed - first_changed + 1;
    if (changed_span >= 4 and changed * 2 >= changed_span) {
      emit_whole_row(y, first_changed, last_changed + 1);
    } else {
      emit_row(y, first, last);
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
