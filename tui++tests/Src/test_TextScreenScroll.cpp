// End-to-end test of the TextScreen terminal-side scroll optimization.
//
// A real window tree (Frame + ScrollPane + TextArea, as the demos build it)
// is shown on the text screen, and the escape stream the screen emits is
// captured and replayed onto a small VT terminal model (cursor moves, SGR,
// scroll regions and insert/delete line are interpreted). Scrolling the
// viewport must emit a terminal scroll (CSI Ps M / CSI Ps L) and only the
// rows that entered the band, and the model built from those incremental
// paints must be identical to the model of a full re-emission of the same
// frame (TextScreen::clear + refresh) -- the incremental terminal-side
// scrolls must reproduce exactly what a dumb full repaint would print.

#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/ScrollPane.h>
#include <tui++/Screen.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/Viewport.h>
#include <tui++/event/InvocationEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace tui;

namespace {

// ---------------------------------------------------------------------------
// a minimal VT decoder: the subset of the terminal state the text screen
// emits (cursor moves, SGR, DECSTBM, IL/DL, printable glyphs)

struct VtModel {
  int rows;
  int cols;
  // One glyph (a possibly multi-byte UTF-8 character) per cell, so the
  // model's column indices are cell indices like the escape positions.
  std::vector<std::vector<std::string>> cell;
  int cursor_row = 0;
  int cursor_col = 0;
  int region_top = 0;    // DECSTBM, 0-based inclusive
  int region_bottom = 0;

  explicit VtModel(int rows, int cols) :
      rows(rows), cols(cols), region_bottom(rows - 1), cell(std::size_t(rows), std::vector<std::string>(std::size_t(cols), " ")) {
  }

  std::string row_text(int y) const {
    std::string text;
    for (auto const &glyph : this->cell[std::size_t(y)]) {
      text += glyph;
    }
    return text;
  }

  void blank_row(int y) {
    for (auto &glyph : this->cell[std::size_t(y)]) {
      glyph = " ";
    }
  }

  void print(std::string const &glyph) {
    this->cell[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = glyph;
    if (++this->cursor_col >= this->cols) {
      // No emission wraps in practice (the emitter positions every run), but
      // keep the model from indexing out of bounds either way.
      this->cursor_col = 0;
      if (this->cursor_row < this->region_bottom) {
        ++this->cursor_row;
      }
    }
  }

  void move_cursor(int row, int col) { // 1-based parameters
    this->cursor_row = std::clamp(row - 1, 0, this->rows - 1);
    this->cursor_col = std::clamp(col - 1, 0, this->cols - 1);
  }

  void set_region(std::vector<int> const &params) {
    auto top = params.size() > 0 and params[0] > 0 ? params[0] : 1;
    auto bottom = params.size() > 1 and params[1] > 0 ? params[1] : this->rows;
    this->region_top = std::clamp(top - 1, 0, this->rows - 1);
    this->region_bottom = std::clamp(bottom - 1, this->region_top, this->rows - 1);
  }

  // CSI Ps M: delete Ps lines at the cursor, within the scroll region; blank
  // lines appear at the region's bottom edge.
  void delete_lines(int n) {
    auto first = std::clamp(this->cursor_row, this->region_top, this->region_bottom);
    n = std::clamp(n, 0, this->region_bottom - first + 1);
    for (auto y = first; y <= this->region_bottom - n; ++y) {
      this->cell[std::size_t(y)] = this->cell[std::size_t(y + n)];
    }
    for (auto y = this->region_bottom - n + 1; y <= this->region_bottom; ++y) {
      blank_row(y);
    }
  }

  // CSI Ps L: insert Ps blank lines at the cursor; the content of the scroll
  // region shifts down and its bottom rows are discarded.
  void insert_lines(int n) {
    auto first = std::clamp(this->cursor_row, this->region_top, this->region_bottom);
    n = std::clamp(n, 0, this->region_bottom - first + 1);
    for (auto y = this->region_bottom; y >= first + n; --y) {
      this->cell[std::size_t(y)] = this->cell[std::size_t(y - n)];
    }
    for (auto y = first; y < first + n; ++y) {
      blank_row(y);
    }
  }

  // Applies an escape stream to the model. Returns false when a CSI sequence
  // the text screen must not emit was seen.
  bool apply(std::string const &stream) {
    enum class State { TEXT, ESC, CSI };
    auto state = State::TEXT;
    auto params = std::vector<int> { };
    auto param = 0;
    auto have_param = false;
    auto i = std::size_t { 0 };
    while (i < stream.size()) {
      auto c = static_cast<unsigned char>(stream[i]);
      switch (state) {
      case State::TEXT:
        if (c == 0x1B) {
          state = State::ESC;
          ++i;
        } else if (c >= 0x20) {
          // A printable glyph: a UTF-8 sequence is one cell.
          auto len = std::size_t { 1 };
          if (c >= 0xF0) {
            len = 4;
          } else if (c >= 0xE0) {
            len = 3;
          } else if (c >= 0xC0) {
            len = 2;
          }
          len = std::min(len, stream.size() - i);
          print(stream.substr(i, len));
          i += len;
        } else {
          ++i;
        }
        break;

      case State::ESC:
        if (c == '[') {
          state = State::CSI;
          params.clear();
          param = 0;
          have_param = false;
        } else {
          state = State::TEXT; // unknown escape: skip its final byte
        }
        ++i;
        break;

      case State::CSI: {
        if (c >= '0' and c <= '9') {
          param = param * 10 + (c - '0');
          have_param = true;
        } else if (c == ';') {
          params.push_back(have_param ? param : 0);
          param = 0;
          have_param = false;
        } else {
          // The final byte.
          if (have_param) {
            params.push_back(param);
          }
          switch (c) {
          case 'm': // SGR: not needed to track characters
          case 'h':
          case 'l': // modes
          case '?':
            break;
          case 'H':
          case 'f':
            move_cursor(params.size() > 0 ? params[0] : 1, params.size() > 1 ? params[1] : 1);
            break;
          case 'r':
            set_region(params);
            break;
          case 'M':
            delete_lines(params.size() > 0 and params[0] > 0 ? params[0] : 1);
            break;
          case 'L':
            insert_lines(params.size() > 0 and params[0] > 0 ? params[0] : 1);
            break;
          default:
            // The text screen should not emit anything else; flag it so the
            // test fails loudly instead of trusting a half-understood model.
            return false;
          }
          state = State::TEXT;
        }
        ++i;
        break;
      }
      }
    }
    return true;
  }
};

// ---------------------------------------------------------------------------

std::string text_of(std::shared_ptr<TextBuffer> const &buffer) {
  return buffer->read(0, buffer->length());
}

std::uint64_t offset_of_line(std::string const &text, int line) {
  auto pos = std::size_t { 0 };
  for (auto i = 0; i < line; ++i) {
    pos = text.find('\n', pos) + 1;
  }
  return std::uint64_t(pos);
}

// Pops and dispatches the queued repaint invocations (the only events the
// test itself generates); everything else is ignored the way the tests always
// have been (no windows' listeners need them for painting).
void drain_events() {
  auto &queue = screen.get_event_queue();
  for (auto i = 0; i < 2000; ++i) {
    auto event = queue.pop(std::chrono::milliseconds(2));
    if (not event) {
      return;
    }
    if (event->id == InvocationEvent::INVOCATION) {
      static_cast<InvocationEvent&>(*event).dispatch();
    }
  }
  assert(!"event queue did not drain");
}

} // namespace

void test_TextScreen_scroll() {
  std::fprintf(stderr, "test_TextScreen_scroll: painting a scrollable text frame on the text screen\n");

  // The screen emits through std::cout; capture the whole run in one buffer
  // and clear it between phases. Assertions print to stderr, so the capture
  // stays clean.
  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());

  // The earlier tests may have left a repaint invocation pending on the
  // shared screen (they damage and never drain); repaint_damaged resets the
  // flag, so the repaints this test schedules below are actually posted.
  screen.repaint_damaged();
  capture.str({ });

  auto take = [&] {
    auto bytes = capture.str();
    capture.str({ });
    return bytes;
  };

  auto dim = screen.get_size();
  assert(dim.width > 8 and dim.height > 6);

  // A demo-like document with rows that differ from each other in every
  // cell, so a vertical scroll is the only explanation of a shifted band and
  // the per-run fallback costs a full band.
  auto buffer = TextBuffer::create_empty();
  auto text = std::string { };
  for (auto i = 0; i < 200; ++i) {
    char line[160];
    auto n = std::snprintf(line, sizeof line, "line %03d: ", i);
    for (auto j = 0; j < 60; ++j) {
      line[std::size_t(n) + std::size_t(j)] = char('a' + ((i * 13 + j * 7) % 26));
    }
    line[std::size_t(n) + 60] = '\n';
    text += std::string_view { line, std::size_t(n) + 61 };
  }
  buffer->replace(0, 0, text);
  buffer->scan_to_end();

  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("scroll test frame");
  auto pane = make_component<ScrollPane>();
  pane->set_name("scroll test pane");
  frame->add(pane);
  auto area = make_component<TextArea>();
  area->set_buffer(buffer);
  pane->set_viewport_view(area);
  frame->set_visible(true);
  area->request_input_focus();
  drain_events();
  (void)take(); // the show's first paint may differ from steady-state paints

  // Baseline: re-emit the frame from scratch (clear + refresh) and decode it
  // into the terminal model every later phase builds on. This also leaves the
  // shadow fully synced, so the phases below flush like a running app.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto full_paint = take();
  assert(full_paint.size() > 400 && "the full paint must emit the whole screen");

  // Replay the stream on a fresh model; every later phase builds on it, and
  // the final model must equal the model of the final full re-emission.
  auto model = VtModel { dim.height, dim.width };
  assert(model.apply(full_paint));

  // Oracle check: the incremental model must equal a full re-emission of the
  // current view (decoded into a scratch model).
  auto verify_phase = [&](char const *what) {
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    auto reemit = take();
    auto oracle = VtModel { dim.height, dim.width };
    assert(oracle.apply(reemit));
    for (auto row = 0; row < dim.height; ++row) {
      if (oracle.cell[std::size_t(row)] != model.cell[std::size_t(row)]) {
        auto &a = model.cell[std::size_t(row)];
        auto &b = oracle.cell[std::size_t(row)];
        auto col = 0;
        while (col < int(a.size()) and col < int(b.size()) and a[std::size_t(col)] == b[std::size_t(col)]) {
          ++col;
        }
        std::fprintf(stderr, "phase %s: row %d differs after the incremental scrolls (first diff col %d):\n  incremental: %s\n  full repaint: %s\n", what, row, col, model.row_text(row).c_str(), oracle.row_text(row).c_str());
        break;
      }
    }
  };

  // Keep the caret on a visible row for the scrolls that follow.
  area->set_caret(offset_of_line(text, 8));
  drain_events();
  auto caret_paint = take();
  assert(model.apply(caret_paint));

  auto viewport = pane->get_viewport();
  assert(viewport != nullptr);

  // Jump far enough down that no terminal scroll can explain the change
  // (the per-run fallback repaints the band) and remember its cost as the
  // full-band reference.
  auto y = viewport->get_view_position().y;
  viewport->set_view_position(0, y + 25);
  drain_events();
  auto jump_paint = take();
  assert(jump_paint.size() > 400 && "a content jump repaints the visible band");
  assert(model.apply(jump_paint));
  auto const full_ref = jump_paint.size();
  verify_phase("jump");

  // Then scroll three rows at a time, down and back up. Each notch must be a
  // terminal-side scroll of 3 rows (CSI 3 M deletes lines for content that
  // moved up, CSI 3 L inserts them for content that moved down) plus only the
  // rows that entered the band -- far fewer bytes than the band itself.
  auto scrolled_down_bytes = std::size_t { 0 };
  auto scrolled_up_bytes = std::size_t { 0 };
  auto scroll_by = [&](int delta, std::string const &expect, std::size_t &sink) {
    viewport->set_view_position(0, viewport->get_view_position().y + delta);
    drain_events();
    auto paint = take();
    assert(paint.find("\x1b[" + expect) != std::string::npos && "a 3-row scroll must scroll the terminal by 3 rows");
    assert(paint.size() * 2 < full_ref && "a 3-row scroll must cost far less than a full-band repaint");
    assert(model.apply(paint));
    sink += paint.size();
  };

  scroll_by(3, "3M", scrolled_down_bytes);
  verify_phase("down1");
  scroll_by(3, "3M", scrolled_down_bytes);
  verify_phase("down2");
  scroll_by(-3, "3L", scrolled_up_bytes);
  verify_phase("up1");
  scroll_by(-3, "3L", scrolled_up_bytes);
  verify_phase("up2");
  assert(viewport->get_view_position().y == y + 25 && "the viewport is back where the jump put it");

  // A repaint of unchanged content must emit nothing at all (the shadow and
  // the terminal agree after the scrolls).
  pane->repaint();
  drain_events();
  auto noop_paint = take();
  assert(noop_paint.empty() && "a no-op repaint must emit nothing");

  // The oracle: re-emit the same frame from scratch and compare the models.
  // The incremental scrolls must have reproduced exactly the image a dumb
  // full repaint prints.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto full_reemit = take();
  assert(full_reemit.size() > 400 && "the full re-emission must paint the whole screen");

  auto oracle = VtModel { dim.height, dim.width };
  assert(oracle.apply(full_reemit));
  for (auto row = 0; row < dim.height; ++row) {
    if (oracle.cell[std::size_t(row)] != model.cell[std::size_t(row)]) {
      std::fprintf(stderr, "row %d differs after the incremental scrolls:\n  incremental: %s\n  full repaint: %s\n", row, model.row_text(row).c_str(), oracle.row_text(row).c_str());
    }
    assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "the terminal-side scrolls must reproduce the full-repaint image");
  }

  // Remove this test's window from the screen: the windows of the shown
  // frames stay alive in the screen's window list, and a later screen test
  // would otherwise measure its repaints on top of this one's content.
  frame->set_visible(false);
  drain_events();

  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_scroll: full paint %zu bytes, full-band jump %zu bytes, 3-row scrolls down %zu / up %zu bytes, images identical\n",
      full_paint.size(), full_ref, scrolled_down_bytes, scrolled_up_bytes);
}
