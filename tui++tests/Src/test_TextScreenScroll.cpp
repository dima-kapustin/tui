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

#include <tui++/BorderLayout.h>
#include <tui++/BoxLayout.h>
#include <tui++/Button.h>
#include <tui++/ComboBox.h>
#include <tui++/Component.h>
#include <tui++/FlowLayout.h>
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/ScrollPane.h>
#include <tui++/Screen.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/Viewport.h>
#include <tui++/event/InvocationEvent.h>
#include <tui++/event/MouseEvent.h>
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
  // The cell's colors and attributes, as the canonical text "fg=..;bg=..;a=.."
  // the SGR sequences in effect put there (a glyph's appearance, not the
  // escape stream that produced it).
  std::vector<std::vector<std::string>> style;
  int cursor_row = 0;
  int cursor_col = 0;
  int region_top = 0;    // DECSTBM, 0-based inclusive
  int region_bottom = 0;

  explicit VtModel(int rows, int cols) :
      rows(rows), cols(cols), region_bottom(rows - 1),
      cell(std::size_t(rows), std::vector<std::string>(std::size_t(cols), " ")),
      style(std::size_t(rows), std::vector<std::string>(std::size_t(cols), "fg=-;bg=-;a=-")) {
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

  // The SGR state the cells printed from now on carry. Only what the text
  // screen emits is interpreted: a reset, the attributes it uses, and the
  // truecolor foreground/background. The canonical text keeps two streams
  // that leave the same appearance comparable even when they set it
  // differently.
  std::string sgr_state = "fg=-;bg=-;a=-";
  std::string fg = "-";
  std::string bg = "-";
  std::string attributes;

  void set_sgr(std::vector<int> const &params) {
    auto i = std::size_t { 0 };
    auto apply_code = [&](int code) {
      if (code == 0) {
        this->fg = "-";
        this->bg = "-";
        this->attributes.clear();
      } else if (code == 1 or code == 4) {
        auto name = code == 1 ? "b" : "u";
        if (this->attributes.find(name) == std::string::npos) {
          this->attributes += name;
        }
      } else if (code == 22 or code == 24) {
        auto name = code == 22 ? "b" : "u";
        if (auto pos = this->attributes.find(name); pos != std::string::npos) {
          this->attributes.erase(pos, 1);
        }
      } else if (code == 7) {
        if (this->attributes.find('7') == std::string::npos) {
          this->attributes += '7';
        }
      } else if (code == 27) {
        if (auto pos = this->attributes.find('7'); pos != std::string::npos) {
          this->attributes.erase(pos, 1);
        }
      } else if (code == 39) {
        this->fg = "-";
      } else if (code == 49) {
        this->bg = "-";
      } else if (code >= 30 and code <= 37) {
        this->fg = std::to_string(code - 30);
      } else if (code >= 40 and code <= 47) {
        this->bg = std::to_string(code - 40);
      }
    };

    while (i < params.size()) {
      auto code = params[i];
      if ((code == 38 or code == 48) and i + 4 < params.size() and params[i + 1] == 2) {
        auto color = std::to_string(params[i + 2]) + "," + std::to_string(params[i + 3]) + "," + std::to_string(params[i + 4]);
        (code == 38 ? this->fg : this->bg) = color;
        i += 5;
      } else {
        apply_code(code);
        ++i;
      }
    }

    this->sgr_state = "fg=" + this->fg + ";bg=" + this->bg + ";a=" + this->attributes;
  }

  void print(std::string const &glyph) {
    this->cell[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = glyph;
    this->style[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = this->sgr_state;
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
          case 'm': // SGR: the colors and attributes of the cells that follow
            set_sgr(params);
            break;
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

// Pops and dispatches every queued event the way the event loop does, focus
// events and repaint invocations alike (drain_events above drops everything
// but the invocations, which is enough for tests that drive the widgets
// themselves but not for one that has to match what the running application
// does -- a posted focus change repaints its own components).
void pump_events() {
  auto &queue = screen.get_event_queue();
  for (auto i = 0; i < 2000; ++i) {
    auto event = queue.pop(std::chrono::milliseconds(2));
    if (not event) {
      return;
    }
    // What Screen::dispatch_event does with a queued event: hand it to the
    // component it was posted to.
    if (auto target = std::dynamic_pointer_cast<Component>(event->source)) {
      target->dispatch_event(*event);
    } else if (event->id == InvocationEvent::INVOCATION) {
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

  // Then scroll three rows at a time, down and back up. The terminal-side
  // scroll is disabled (its blanked edge flashes for a frame), so every notch
  // is repainted in place: no CSI Ps M / CSI Ps L may reach the terminal, and
  // the band is re-emitted (comparable to a full-band repaint).
  auto scrolled_down_bytes = std::size_t { 0 };
  auto scrolled_up_bytes = std::size_t { 0 };
  auto scroll_by = [&](int delta, std::string const &expect, std::size_t &sink) {
    viewport->set_view_position(0, viewport->get_view_position().y + delta);
    drain_events();
    auto paint = take();
    assert(paint.find("\x1b[" + expect) == std::string::npos && "a scroll must not scroll the terminal");
    assert(paint.size() * 2 < full_ref * 3 && "an in-place scroll must stay within the band's cost");
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

  // A one-row scroll repaints the band in place the same way (the fixed
  // scroll bar column must not be scrolled with the content).
  viewport->set_view_position(0, viewport->get_view_position().y + 1);
  drain_events();
  auto one_row_paint = take();
  assert(one_row_paint.find("\x1b[1M") == std::string::npos and one_row_paint.find("\x1b[1L") == std::string::npos &&
      "a one-row scroll must not scroll the terminal");
  assert(model.apply(one_row_paint));
  verify_phase("one-row");

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
    assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "the in-place scrolls must reproduce the full-repaint image");
  }

  // Remove this test's window from the screen: the windows of the shown
  // frames stay alive in the screen's window list, and a later screen test
  // would otherwise measure its repaints on top of this one's content.
  frame->set_visible(false);
  drain_events();

  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_scroll: full paint %zu bytes, full-band jump %zu bytes, 3-row in-place scrolls down %zu / up %zu bytes, images identical\n",
      full_paint.size(), full_ref, scrolled_down_bytes, scrolled_up_bytes);
}

// A horizontal scroll repaints only the visible window of a long line, not the
// whole natural width: the emission must stay proportional to the viewport,
// and the model must match a full re-emission.
void test_TextScreen_horizontal_scroll() {
  std::fprintf(stderr, "test_TextScreen_horizontal_scroll: horizontal scroll of a long line\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] { auto b = capture.str(); capture.str({ }); return b; };

  auto dim = screen.get_size();
  assert(dim.width > 8 and dim.height > 6);

  // Lines much longer than the viewport, with content that differs per column
  // so a horizontal shift actually changes every visible cell.
  auto buffer = TextBuffer::create_empty();
  std::string text;
  for (auto i = 0; i < 8; ++i) {
    for (auto j = 0; j < 120; ++j) {
      text += char('a' + ((i * 13 + j * 7) % 26));
    }
    text += "\n";
  }
  buffer->replace(0, 0, text);
  buffer->scan_to_end();

  auto frame = make_component<Frame>();
  frame->set_size(dim);
  auto pane = make_component<ScrollPane>();
  frame->add(pane);
  auto area = make_component<TextArea>();
  area->set_buffer(buffer);
  pane->set_viewport_view(area);
  frame->set_visible(true);
  drain_events();
  (void)take();

  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto full_paint = take();
  auto model = VtModel { dim.height, dim.width };
  assert(model.apply(full_paint));

  assert(pane->horizontal_bar_needed());

  auto viewport = pane->get_viewport();
  viewport->set_view_position(40, 0);
  drain_events();
  auto scroll_paint = take();
  std::fprintf(stderr, "test_TextScreen_horizontal_scroll: horizontal scroll emits %zu bytes (full paint %zu)\n", scroll_paint.size(), full_paint.size());
  assert(model.apply(scroll_paint));

  // The horizontal scroll must not repaint the whole pane: it stays well below
  // a full re-emission of the screen.
  assert(scroll_paint.size() * 2 < full_paint.size() && "a horizontal scroll must not repaint the whole screen");

  // Oracle: the incremental scroll must reproduce the full-repaint image.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto reemit = take();
  auto oracle = VtModel { dim.height, dim.width };
  assert(oracle.apply(reemit));
  for (auto row = 0; row < dim.height; ++row) {
    assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "horizontal scroll must reproduce the full-repaint image");
  }

  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_horizontal_scroll: ok\n");
}

// A terminal-side scroll must never scroll a band that an open popup window
// overlaps: the popup's cells are not part of the scrolling surface, so a
// terminal scroll would drag the popup with the content and re-emit it back
// (a flicker). While a popup overlaps the scrolled viewport, a wheel scroll
// must fall back to the per-run repaint and still reproduce the full-repaint
// image.
void test_TextScreen_popup_over_scroll() {
  std::fprintf(stderr, "test_TextScreen_popup_over_scroll: popup overlapping a scroll\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] { auto b = capture.str(); capture.str({ }); return b; };

  auto dim = screen.get_size();
  assert(dim.width > 8 and dim.height > 6);

  auto buffer = TextBuffer::create_empty();
  auto text = std::string { };
  for (auto i = 0; i < 120; ++i) {
    char line[128];
    auto n = std::snprintf(line, sizeof line, "line %03d: ", i);
    for (auto j = 0; j < 50; ++j) {
      line[std::size_t(n) + std::size_t(j)] = char('a' + ((i * 13 + j * 7) % 26));
    }
    line[std::size_t(n) + 50] = '\n';
    text += std::string_view { line, std::size_t(n) + 51 };
  }
  buffer->replace(0, 0, text);
  buffer->scan_to_end();

  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("popup scroll frame");

  auto file_menu = make_component<Menu>("File");
  file_menu->add(make_component<MenuItem>("New"));
  file_menu->add(make_component<MenuItem>("Open"));
  file_menu->add(make_component<MenuItem>("Save"));
  file_menu->add(make_component<MenuItem>("Exit"));
  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(file_menu);
  frame->set_menu_bar(menu_bar);

  auto pane = make_component<ScrollPane>();
  pane->set_name("popup scroll pane");
  frame->add(pane);
  auto area = make_component<TextArea>();
  area->set_buffer(buffer);
  pane->set_viewport_view(area);

  frame->set_visible(true);
  area->request_input_focus();
  drain_events();
  (void)take();

  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto full_paint = take();
  auto model = VtModel { dim.height, dim.width };
  assert(model.apply(full_paint));

  // Open the popup over the scroll area and confirm it overlaps the viewport's
  // vertical band (the guard only matters while a popup actually overlaps).
  file_menu->set_popup_menu_visible(true);
  drain_events();
  (void)take();
  auto popup_window = file_menu->get_popup_menu()->get_containing_window();
  assert(popup_window != nullptr);
  assert(popup_window->is_visible());

  auto viewport = pane->get_viewport();
  assert(viewport != nullptr);
  auto vp_rect = Rectangle { viewport->get_location_on_screen(), viewport->get_size() };
  auto pop_rect = Rectangle { popup_window->get_location(), popup_window->get_size() };
  assert(not (vp_rect & pop_rect).empty() && "the popup must overlap the scroll viewport");

  // Re-baseline with the popup visible, so the scroll phase below starts from
  // a shadow (and model) that already contains the popup's cells.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto with_popup_paint = take();
  model = VtModel { dim.height, dim.width };
  assert(model.apply(with_popup_paint));

  // A 3-row scroll with the popup overlapping must not use a terminal scroll
  // (CSI Ps M / CSI Ps L): scrolling the band would move the popup too.
  viewport->set_view_position(0, viewport->get_view_position().y + 3);
  drain_events();
  auto scroll_paint = take();
  assert(scroll_paint.find("\x1b[3M") == std::string::npos && "a popup-overlapping scroll must not delete lines");
  assert(scroll_paint.find("\x1b[3L") == std::string::npos && "a popup-overlapping scroll must not insert lines");
  assert(model.apply(scroll_paint));

  // The per-run repaint must still reproduce the full-repaint image.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto reemit = take();
  auto oracle = VtModel { dim.height, dim.width };
  assert(oracle.apply(reemit));
  for (auto row = 0; row < dim.height; ++row) {
    if (oracle.cell[std::size_t(row)] != model.cell[std::size_t(row)]) {
      std::fprintf(stderr, "row %d differs:\n  incremental: %s\n  full repaint: %s\n", row, model.row_text(row).c_str(), oracle.row_text(row).c_str());
      break;
    }
  }
  for (auto row = 0; row < dim.height; ++row) {
    assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "popup-overlapping scroll must reproduce the full-repaint image");
  }

  file_menu->set_popup_menu_visible(false);
  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_popup_over_scroll: ok\n");
}

// A status line below the pane repaints when the scroll model moves (the
// TextAreaDemo does this to show "row=N"). That damage is a separate region
// from the viewport, so a scroll repaints the viewport band and the status
// line as separate regions in place -- the terminal-side scroll (whose
// full-width band would drag the fixed rows with the content) is disabled
// entirely, so the fixed UI can never move with a scroll.
void test_TextScreen_scroll_keeps_fixed_ui_out_of_band() {
  std::fprintf(stderr, "test_TextScreen_scroll_keeps_fixed_ui_out_of_band: status line below a scrolling pane\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] { auto b = capture.str(); capture.str({ }); return b; };

  auto dim = screen.get_size();
  assert(dim.width > 8 and dim.height > 6);

  // Lines wider than the viewport so the pane shows a horizontal scroll bar;
  // that bar sits between the viewport and the status line, so a scroll must
  // keep the status line out of the terminal-scroll band.
  auto buffer = TextBuffer::create_empty();
  auto text = std::string { };
  for (auto i = 0; i < 120; ++i) {
    char line[160];
    auto n = std::snprintf(line, sizeof line, "line %03d: ", i);
    for (auto j = 0; j < 120; ++j) {
      line[std::size_t(n) + std::size_t(j)] = char('a' + ((i * 13 + j * 7) % 26));
    }
    line[std::size_t(n) + 120] = '\n';
    text += std::string_view { line, std::size_t(n) + 121 };
  }
  buffer->replace(0, 0, text);
  buffer->scan_to_end();

  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("fixed-ui scroll frame");

  auto pane = make_component<ScrollPane>();
  pane->set_name("fixed-ui scroll pane");
  frame->add(pane);
  auto area = make_component<TextArea>();
  area->set_buffer(buffer);
  pane->set_viewport_view(area);

  // A one-row status line below the pane, repainted when the scroll bar model
  // moves (exactly the TextAreaDemo's status line).
  auto status = make_component<Panel>();
  status->set_preferred_size(Dimension { 0, 1 });
  status->set_background_color(Color { 8, 10, 15 });
  frame->add(status, BorderLayout::SOUTH);
  pane->get_vertical_scroll_bar()->get_model()->add_change_listener([status] {
    status->repaint();
  });

  frame->set_visible(true);
  area->request_input_focus();
  drain_events();
  (void)take();

  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto full_paint = take();
  auto model = VtModel { dim.height, dim.width };
  assert(model.apply(full_paint));

  auto viewport = pane->get_viewport();
  assert(viewport != nullptr);
  assert(pane->horizontal_bar_needed());

  viewport->set_view_position(0, viewport->get_view_position().y + 3);
  drain_events();
  auto scroll_paint = take();
  assert(scroll_paint.find("\x1b[3M") == std::string::npos and scroll_paint.find("\x1b[3L") == std::string::npos &&
      "a scroll must not use the terminal scroll");

  assert(model.apply(scroll_paint));
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto reemit = take();
  auto oracle = VtModel { dim.height, dim.width };
  assert(oracle.apply(reemit));
  for (auto row = 0; row < dim.height; ++row) {
    assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "scroll with a status line must reproduce the full-repaint image");
  }

  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_scroll_keeps_fixed_ui_out_of_band: ok\n");
}

// Closing a popup must erase its rows from the terminal even when the content
// beneath it paints nothing (a combo dropdown over the empty tail of a
// WidgetDemo-style panel). The repaint of the windows underneath only
// overwrites the cells they draw, so the closed dropdown's rows used to stay
// in the screen's back buffer and on the terminal forever; the state after a
// close must equal the pre-open image, row for row.
void test_TextScreen_popup_close_erasure() {
  std::fprintf(stderr, "test_TextScreen_popup_close_erasure: a closed popup erases its rows\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] { auto b = capture.str(); capture.str({ }); return b; };

  auto dim = screen.get_size();
  assert(dim.width > 12 and dim.height > 10);

  // A WidgetDemo-like frame: a BoxLayout column whose FlowLayout row holds
  // the combo box, over the (unpainted) content pane -- the dropdown drops
  // onto cells no component ever paints.
  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("popup erase frame");
  auto content = frame->get_content_pane();
  content->set_layout(std::make_shared<BorderLayout>());
  auto panel = make_component<Panel>();
  panel->set_layout(std::make_shared<BoxLayout>(panel.get(), BoxLayout::Y));
  auto row = make_component<Panel>(); // a FlowLayout row, like the demos'
  auto city = make_component<ComboBox>(std::vector<std::string> { "Paris", "London", "Rome", "Berlin" });
  city->set_selected_index(0);
  city->set_name("city");
  row->add(city);
  panel->add(row);
  content->add(panel, BorderLayout::CENTER);

  frame->set_visible(true);
  drain_events();
  (void)take();

  // The pre-open image, captured into the model the phases below build on.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto closed_paint = take();
  auto model = VtModel { dim.height, dim.width };
  assert(model.apply(closed_paint));
  auto pre_open = model; // the rows the dropdown will cover

  // Open the dropdown: its rows land on the unpainted area below the combo.
  city->set_popup_visible(true);
  drain_events();
  auto open_paint = take();
  auto at = city->get_location_on_screen();
  auto popup_window = screen.get_window_at(at.x + 2, at.y + city->get_height() + 1);
  assert(popup_window != nullptr and popup_window.get() != frame.get() and popup_window->is_visible());
  auto popup_bounds = popup_window->get_bounds();
  assert(popup_bounds.height >= 4 and "the dropdown must be tall enough to be visible");
  assert(model.apply(open_paint));
  auto saw_item = false;
  for (auto y = std::max(popup_bounds.y, 0); y < std::min(popup_bounds.bottom(), dim.height); ++y) {
    if (model.row_text(y).find("Paris") != std::string::npos) {
      saw_item = true;
    }
  }
  assert(saw_item && "the open dropdown must paint its rows on screen");

  // Close it: every row the dropdown covered must be back to the pre-open
  // image. Before the screen dropped the removed window's cells, the rows
  // stayed as the dropdown had painted them (the ghost).
  city->set_popup_visible(false);
  drain_events();
  auto close_paint = take();
  assert(model.apply(close_paint));
  for (auto y = std::max(popup_bounds.y, 0); y < std::min(popup_bounds.bottom(), dim.height); ++y) {
    assert(model.cell[std::size_t(y)] == pre_open.cell[std::size_t(y)] && "closing the dropdown must erase its rows");
  }

  // Oracle: the incremental stream must reproduce the full-repaint image.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto reemit = take();
  auto oracle = VtModel { dim.height, dim.width };
  assert(oracle.apply(reemit));
  for (auto row = 0; row < dim.height; ++row) {
    assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "closing the dropdown must reproduce the full-repaint image");
  }

  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_popup_close_erasure: ok\n");
}

// A repaint whose region only covers part of a bordered component must draw
// the border cells it covers -- the cells at the region's own edges included.
// The rect painter stepped over them (it assumed the box's own corner would
// be drawn there), so a partial repaint left the border cell at the region's
// edge as a blank: the black square a closed dropdown used to leave on a
// frame's border.
void test_TextScreen_clipped_border_repaint() {
  std::fprintf(stderr, "test_TextScreen_clipped_border_repaint: a partial repaint redraws the border cells it covers\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] {
    auto bytes = capture.str();
    capture.str({ });
    return bytes;
  };

  auto dim = screen.get_size();
  assert(dim.width > 20 and dim.height > 12);

  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("clipped border frame");
  auto panel = make_component<Panel>();
  auto line = make_component<Panel>();
  line->set_preferred_size(Dimension { dim.width - 6, 1 });
  panel->add(line);
  frame->get_content_pane()->add(panel);

  frame->set_visible(true);
  drain_events();
  (void) take();

  // The full-repaint image: the reference every partial repaint must match.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto reference = VtModel { dim.height, dim.width };
  assert(reference.apply(take()));
  assert(reference.row_text(0).find_first_not_of(' ') != std::string::npos && "the frame must paint its top border");

  // Bands that cut each edge of the border, and each corner, in the middle:
  // the repaint of a band must leave every cell it covers as the full repaint
  // draws it.
  auto bands = std::vector<std::pair<char const *, Rectangle>> {
      { "top edge", Rectangle { dim.width / 2, 0, 4, 1 } },
      { "bottom edge", Rectangle { dim.width / 2, dim.height - 1, 4, 1 } },
      { "left edge", Rectangle { 0, dim.height / 2, 1, 3 } },
      { "right edge", Rectangle { dim.width - 1, dim.height / 2, 1, 3 } },
      { "top left corner", Rectangle { 0, 0, 4, 1 } },
      { "top right corner", Rectangle { dim.width - 4, 0, 4, 1 } },
      { "bottom left corner", Rectangle { 0, dim.height - 1, 4, 1 } },
      { "bottom right corner", Rectangle { dim.width - 4, dim.height - 1, 4, 1 } },
  };

  for (auto const &[what, band] : bands) {
    frame->repaint(band);
    drain_events();
    auto incremental = reference;
    assert(incremental.apply(take()));

    for (auto y = band.y; y < band.bottom(); ++y) {
      for (auto x = band.x; x < band.right(); ++x) {
        if (incremental.cell[std::size_t(y)][std::size_t(x)] != reference.cell[std::size_t(y)][std::size_t(x)]) {
          std::fprintf(stderr, "%s: cell (%d, %d) is '%s', the border has '%s'\n  repainted: %s\n  border:    %s\n",
              what, x, y, incremental.cell[std::size_t(y)][std::size_t(x)].c_str(), reference.cell[std::size_t(y)][std::size_t(x)].c_str(),
              incremental.row_text(y).c_str(), reference.row_text(y).c_str());
        }
        assert(incremental.cell[std::size_t(y)][std::size_t(x)] == reference.cell[std::size_t(y)][std::size_t(x)] && "a partial repaint must redraw the border cells it covers");
      }
    }

    // Nothing outside the band may change.
    for (auto y = 0; y < dim.height; ++y) {
      assert(incremental.cell[std::size_t(y)] == reference.cell[std::size_t(y)] && "a partial repaint must not touch anything outside its region");
    }
  }

  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_clipped_border_repaint: ok\n");
}

// Clicking the combo box arrow open and closed again must leave every cell of
// the screen as it was -- the frame's own border and title included, not only
// the rows the dropdown covered. (Dropping a removed window's cells already
// restores the rows under the dropdown; what this guards is the whole screen,
// where a cell the repaint failed to restore shows as a blank block.)
void test_TextScreen_combo_arrow_repaint() {
  std::fprintf(stderr, "test_TextScreen_combo_arrow_repaint: the arrow open/close cycle leaves the screen untouched\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] {
    auto bytes = capture.str();
    capture.str({ });
    return bytes;
  };

  auto dim = screen.get_size();
  assert(dim.width > 20 and dim.height > 12);

  // A WidgetDemo-like frame: a framed window with a menu bar and a row of
  // widgets, a combo box among them.
  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("combo repaint frame");

  auto menu_bar = make_component<MenuBar>();
  auto file_menu = make_component<Menu>("File");
  menu_bar->add(file_menu);
  frame->set_menu_bar(menu_bar);

  auto content = frame->get_content_pane();
  content->set_layout(std::make_shared<BorderLayout>());
  auto panel = make_component<Panel>();
  panel->set_layout(std::make_shared<BoxLayout>(panel.get(), BoxLayout::Y));
  auto row = make_component<Panel>(); // a FlowLayout row, like the demos'
  auto city = make_component<ComboBox>(std::vector<std::string> { "Paris", "London", "Rome", "Berlin" });
  city->set_selected_index(0);
  city->set_name("city");
  auto size = make_component<ComboBox>(std::vector<std::string> { "8 pt", "10 pt", "12 pt" });
  size->set_selected_index(0);
  size->set_name("size");
  row->add(city);
  row->add(size);
  panel->add(row);
  content->add(panel, BorderLayout::CENTER);

  frame->set_visible(true);
  drain_events();
  (void) take();

  // The pre-open image: the whole screen, the frame's border included.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto model = VtModel { dim.height, dim.width };
  assert(model.apply(take()));
  auto pre_open = model;
  assert(model.row_text(0).find_first_not_of(' ') != std::string::npos && "the frame must paint its top border");

  // One arrow click: the pointer moves onto the arrow strip (the box's
  // rightmost cell) and presses and releases there, dispatched through the
  // window as the terminal does.
  auto click_arrow = [&](std::shared_ptr<ComboBox> const &box) {
    pump_events();
    auto at = box->get_location_on_screen();
    auto local = convert_point_from_screen(Point { at.x + box->get_width() - 1, at.y + box->get_height() / 2 }, frame);
    screen.post<MouseMoveEvent>(frame, InputEvent::NO_MODIFIERS, local.x, local.y);
    frame->dispatch_event(*screen.get_event_queue().pop());
    screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, local.x, local.y, false);
    frame->dispatch_event(*screen.get_event_queue().pop());
    screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, local.x, local.y, false);
    frame->dispatch_event(*screen.get_event_queue().pop());
    // The focus changes the press asked for are posted: the running
    // application dispatches them on its next loop, before it paints.
    pump_events();
  };

  // Every cell must be back to the pre-open image -- the glyph and the colors
  // and attributes it is drawn in, not just the rows the dropdown covered.
  auto same_as_pre_open = [&](char const *what) {
    for (auto y = 0; y < dim.height; ++y) {
      for (auto x = 0; x < dim.width; ++x) {
        auto const &now = model.cell[std::size_t(y)][std::size_t(x)];
        auto const &was = pre_open.cell[std::size_t(y)][std::size_t(x)];
        auto const &now_style = model.style[std::size_t(y)][std::size_t(x)];
        auto const &was_style = pre_open.style[std::size_t(y)][std::size_t(x)];
        if (now != was or now_style != was_style) {
          std::fprintf(stderr, "%s: cell (%d, %d) is '%s' [%s], was '%s' [%s]\n  after:  %s\n  before: %s\n",
              what, x, y, now.c_str(), now_style.c_str(), was.c_str(), was_style.c_str(),
              model.row_text(y).c_str(), pre_open.row_text(y).c_str());
        }
        assert(now == was && "closing the dropdown must restore the whole screen");
        assert(now_style == was_style && "closing the dropdown must restore every cell's appearance");
      }
    }
  };

  // Open the city dropdown with the arrow, close it again.
  click_arrow(city);
  assert(city->is_popup_visible() && "the arrow click must open the dropdown");
  assert(model.apply(take()));
  click_arrow(city);
  assert(not city->is_popup_visible() && "the second arrow click must close the dropdown");
  assert(model.apply(take()));
  same_as_pre_open("open/close");

  // Move the dropdown from one box to the other with the arrows, then close
  // it: the first box's dropdown is dismissed as the second one opens.
  click_arrow(city);
  assert(model.apply(take()));
  click_arrow(size);
  assert(not city->is_popup_visible() and size->is_popup_visible() && "the second box's arrow moves the dropdown to it");
  assert(model.apply(take()));
  click_arrow(size);
  assert(not size->is_popup_visible() && "the second box's second arrow click closes the dropdown");
  assert(model.apply(take()));
  same_as_pre_open("two boxes");

  // The oracle: a full repaint must show the same screen the incremental
  // stream left (glyphs and appearances alike).
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto oracle = VtModel { dim.height, dim.width };
  assert(oracle.apply(take()));
  for (auto y = 0; y < dim.height; ++y) {
    assert(oracle.cell[std::size_t(y)] == model.cell[std::size_t(y)] && "the incremental paints must reproduce the full-repaint image");
    assert(oracle.style[std::size_t(y)] == model.style[std::size_t(y)] && "the incremental paints must reproduce the full-repaint appearance");
  }

  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_combo_arrow_repaint: ok\n");
}

// A repaint after the terminal's content became unknown -- the first paint, a
// resize -- must write every row in full, clearing the cells the tree does not
// paint. The cells of a layout the app has left behind used to keep whatever
// the terminal showed there, so after a resize the old widget positions stayed
// on screen as dirt and the widgets seemed never to move back. The same stream
// applied to a terminal full of stale content and to a blank terminal must
// produce the same screen.
void test_TextScreen_repaint_over_unknown_content() {
  std::fprintf(stderr, "test_TextScreen_repaint_over_unknown_content: a repaint determines the whole screen\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] { auto b = capture.str(); capture.str({ }); return b; };

  auto dim = screen.get_size();
  assert(dim.width > 8 and dim.height > 6);

  // A tree that paints only part of the screen: a button box in a corner, the
  // rest left to the terminal's background -- a WidgetDemo-style panel whose
  // rows of controls leave most of the cells transparent.
  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("repaint frame");
  auto content = frame->get_content_pane();
  content->set_layout({ }); // explicit bounds, no layout manager
  auto button = make_component<Button>("Go");
  button->set_bounds(2, 2, 8, 3);
  content->add(button);

  frame->set_visible(true);
  drain_events();
  (void)take();

  // The pre-resize layout: a full paint from a known state, applied to the
  // (blank) terminal model.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto before = take();
  auto terminal = VtModel { dim.height, dim.width };
  assert(terminal.apply(before));

  // "The resize": the terminal's content became unknown and the layout moved
  // the widget. clear() leaves the screen exactly like TextScreen::resized
  // does -- the view and the shadow are reset, every row is unsent -- so the
  // refresh below is the resize's full repaint.
  button->set_bounds(2, dim.height - 8, 8, 3);
  drain_events();
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto after = take();
  assert(terminal.apply(after)); // the repaint lands on the stale terminal

  // Oracle: the same stream on a blank terminal must give the same screen.
  auto clean = VtModel { dim.height, dim.width };
  assert(clean.apply(after));
  for (auto row = 0; row < dim.height; ++row) {
    if (clean.cell[std::size_t(row)] != terminal.cell[std::size_t(row)]) {
      std::fprintf(stderr, "row %d differs:\n  stale terminal: %s\n  blank terminal: %s\n", row, terminal.row_text(row).c_str(), clean.row_text(row).c_str());
      break;
    }
  }
  for (auto row = 0; row < dim.height; ++row) {
    assert(clean.cell[std::size_t(row)] == terminal.cell[std::size_t(row)] && "a repaint must clear the cells the tree no longer paints");
  }

  // A widget that moves must not leave its old cells behind either: the
  // damaged-region repaint that follows the move must reproduce the full
  // repaint of the new state.
  {
    auto moved_capture = std::ostringstream { };
    auto *old = std::cout.rdbuf(moved_capture.rdbuf());
    button->set_bounds(2, (dim.height - 8) / 2, 8, 3);
    drain_events();
    auto moved = moved_capture.str();
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    auto full = moved_capture.str().substr(moved.size());
    std::cout.rdbuf(old);

    auto incremental = terminal; // the screen before the move
    assert(incremental.apply(moved));
    auto oracle = VtModel { dim.height, dim.width };
    assert(oracle.apply(full));
    for (auto row = 0; row < dim.height; ++row) {
      if (incremental.cell[std::size_t(row)] != oracle.cell[std::size_t(row)]) {
        std::fprintf(stderr, "move: row %d differs:\n  incremental: %s\n  full paint : %s\n", row, incremental.row_text(row).c_str(), oracle.row_text(row).c_str());
        break;
      }
    }
    for (auto row = 0; row < dim.height; ++row) {
      assert(incremental.cell[std::size_t(row)] == oracle.cell[std::size_t(row)] && "a moved widget must not leave its old cells behind");
    }
  }

  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextScreen_repaint_over_unknown_content: ok\n");
}
