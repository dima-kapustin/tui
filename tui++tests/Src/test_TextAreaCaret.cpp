// Tests of the TextArea caret and of the row-granular repaint damage.
//
// Like test_TextScreenScroll it shows a real window tree (Frame + ScrollPane
// + TextArea) on the text screen, captures the emitted escape stream and
// replays it onto a small VT model. The checks:
//   * typing a character into a visible row repaints that row only -- a
//     small fraction of a full-band repaint, and the model stays identical
//     to a full re-emission of the frame (the oracle);
//   * inserting a line (Enter) repaints from the edited row down and stays
//     identical to the oracle as well;
//   * a caret move emits just the band between the old and the new caret
//     rows;
//   * the caret blinks: the timer toggles the caret cell on and off with
//     small emissions, and the caret is not drawn while the area does not
//     own the keyboard focus.

#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/ScrollPane.h>
#include <tui++/Screen.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/Viewport.h>
#include <tui++/event/InvocationEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace tui;

namespace {

// ---------------------------------------------------------------------------
// a minimal VT decoder (same model as test_TextScreenScroll): cursor moves,
// SGR, DECSTBM and IL/DL, enough to replay the text screen's emission

struct VtModel {
  int rows;
  int cols;
  std::vector<std::vector<std::string>> cell;
  // Whether each cell carries the inverse (standout) attribute -- the block
  // caret paints itself that way.
  std::vector<std::vector<bool>> inverse;
  int cursor_row = 0;
  int cursor_col = 0;
  int region_top = 0;
  int region_bottom = 0;
  bool inv = false;

  explicit VtModel(int rows, int cols) :
      rows(rows), cols(cols), region_bottom(rows - 1), cell(std::size_t(rows), std::vector<std::string>(std::size_t(cols), " ")), inverse(std::size_t(rows), std::vector<bool>(std::size_t(cols), false)) {
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
    std::fill(this->inverse[std::size_t(y)].begin(), this->inverse[std::size_t(y)].end(), false);
  }

  void print(std::string const &glyph) {
    this->cell[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = glyph;
    this->inverse[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = this->inv;
    if (++this->cursor_col >= this->cols) {
      this->cursor_col = 0;
      if (this->cursor_row < this->region_bottom) {
        ++this->cursor_row;
      }
    }
  }

  void move_cursor(int row, int col) {
    this->cursor_row = std::clamp(row - 1, 0, this->rows - 1);
    this->cursor_col = std::clamp(col - 1, 0, this->cols - 1);
  }

  void set_region(std::vector<int> const &params) {
    auto top = params.size() > 0 and params[0] > 0 ? params[0] : 1;
    auto bottom = params.size() > 1 and params[1] > 0 ? params[1] : this->rows;
    this->region_top = std::clamp(top - 1, 0, this->rows - 1);
    this->region_bottom = std::clamp(bottom - 1, this->region_top, this->rows - 1);
  }

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
          state = State::TEXT;
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
          if (have_param) {
            params.push_back(param);
          }
          switch (c) {
          case 'm': {
            // SGR: 0 resets, 7 sets inverse (the block caret), 27 resets it.
            if (params.empty()) {
              this->inv = false;
            } else {
              for (auto p : params) {
                if (p == 0 or p == 27) {
                  this->inv = false;
                } else if (p == 7) {
                  this->inv = true;
                }
              }
            }
            break;
          }
          case 'h':
          case 'l':
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

std::uint64_t offset_of_line(std::string const &text, int line) {
  auto pos = std::size_t { 0 };
  for (auto i = 0; i < line; ++i) {
    pos = text.find('\n', pos) + 1;
  }
  return std::uint64_t(pos);
}

// Dispatches the queued repaint invocations (the only events the test itself
// generates); anything else is ignored.
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

// Dispatches one key event through the window, the way the terminal does
// (keys go to the focused window; the area receives them through its
// forwarder).
void type_key(std::shared_ptr<Frame> const &frame, KeyEvent::Type type, KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) {
  screen.post<KeyEvent>(frame, type, key_code, modifiers);
  auto event = screen.get_event_queue().pop();
  assert(event != nullptr);
  frame->dispatch_event(*event);
  drain_events();
}

void type_char(std::shared_ptr<Frame> const &frame, Char const &character) {
  screen.post<KeyEvent>(frame, character, InputEvent::NO_MODIFIERS);
  auto event = screen.get_event_queue().pop();
  assert(event != nullptr);
  frame->dispatch_event(*event);
  drain_events();
}

} // namespace

void test_TextArea_caret() {
  std::fprintf(stderr, "test_TextArea_caret: typing, caret movement and the blink on the text screen\n");

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
  assert(dim.width > 8 and dim.height > 6);

  // Rows with content that differs everywhere, so row damage is measurable.
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
  frame->set_name("caret test frame");
  auto pane = make_component<ScrollPane>();
  pane->set_name("caret test pane");
  frame->add(pane);
  auto area = make_component<TextArea>();
  area->set_buffer(buffer);
  pane->set_viewport_view(area);
  frame->set_visible(true);
  area->request_input_focus();
  drain_events();
  (void)take(); // the show's first paint may differ from steady-state paints

  // Steady caret for the phases below (the blink is driven explicitly later).
  area->set_caret_blink_rate(std::chrono::milliseconds::zero());

  // Baseline: re-emit the frame from scratch and decode it; every later phase
  // builds on this model and must end up identical to a full re-emission.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto full_paint = take();
  assert(full_paint.size() > 400 && "the full paint must emit the whole screen");
  auto model = VtModel { dim.height, dim.width };
  assert(model.apply(full_paint));

  auto verify_phase = [&](char const *what) {
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    auto reemit = take();
    auto oracle = VtModel { dim.height, dim.width };
    assert(oracle.apply(reemit));
    for (auto row = 0; row < dim.height; ++row) {
      if (oracle.cell[std::size_t(row)] != model.cell[std::size_t(row)]) {
        std::fprintf(stderr, "phase %s: row %d differs:\n  incremental: %s\n  full repaint: %s\n",
            what, row, model.row_text(row).c_str(), oracle.row_text(row).c_str());
        assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "incremental paints must reproduce the full-repaint image");
      }
    }
  };

  // The caret on a mid-screen row (line 8, column 5).
  auto line8_start = offset_of_line(text, 8);
  area->set_caret(line8_start + 5);
  drain_events();
  auto caret_paint = take();
  assert(caret_paint.size() > 0 && "moving the caret must emit its cell");
  assert(caret_paint.size() * 4 < full_paint.size() && "a caret move must not repaint the screen");
  assert(model.apply(caret_paint));

  // Typing one character edits a single row: the emission must stay far
  // below a full-band repaint, and the model must match the oracle.
  type_char(frame, Char { 'x' });
  auto type_paint = take();
  assert(model.apply(type_paint));
  assert(type_paint.size() > 0 && "typing must emit the edited row");
  assert(type_paint.size() * 4 < full_paint.size() && "typing one character must repaint one row, not the screen");
  verify_phase("typed character");

  // A second character into the same row keeps the cost of one row.
  type_char(frame, Char { 'y' });
  auto type2_paint = take();
  assert(type2_paint.size() * 4 < full_paint.size() && "a second character still repaints one row");
  assert(model.apply(type2_paint));

  // Typing at the very start of a line shifts its whole tail: the edited row
  // must be emitted as one contiguous run (one cursor move), not a fragmented
  // "snake" of many small runs.
  area->set_caret(offset_of_line(text, 8));
  drain_events();
  auto caret_at_start = take();
  assert(model.apply(caret_at_start));
  type_char(frame, Char { 'Z' });
  auto start_type_paint = take();
  assert(model.apply(start_type_paint));
  {
    auto moves = std::count(start_type_paint.begin(), start_type_paint.end(), 'H');
    if (moves > 1) {
      std::fprintf(stderr, "snake paint (%d H): %s\n", int(moves), start_type_paint.c_str());
    }
    assert(moves <= 1 && "a line-start edit must emit one run, not a snake");
  }
  verify_phase("line-start");

  // Enter inserts a line: rows below the caret shift down, so the damage
  // runs from the edited row to the bottom; the image must stay correct.
  type_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER, InputEvent::NO_MODIFIERS);
  auto enter_paint = take();
  assert(enter_paint.size() > 0 && "inserting a line must emit the shifted band");
  assert(model.apply(enter_paint));
  verify_phase("inserted line");

  // Shift+Right extends the selection to the next character: only the band
  // between the old and the new caret row is repainted.
  type_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN);
  auto select_paint = take();
  assert(select_paint.size() > 0 && "extending the selection must emit its row");
  assert(select_paint.size() * 4 < full_paint.size() && "a same-row selection move repaints one row");
  assert(model.apply(select_paint));
  verify_phase("selection");

  // A plain (non-extending) caret move collapses the selection again.
  type_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT, InputEvent::NO_MODIFIERS);
  auto move_paint = take();
  assert(move_paint.size() * 4 < full_paint.size() && "a same-row caret move repaints one row");
  assert(model.apply(move_paint));
  verify_phase("caret move");

  // The blink: with a short period the timer toggles the caret cell on and
  // off; every tick emits the caret cell only (SGR plus one glyph), a tiny
  // fraction of the screen.
  area->set_caret_blink_rate(std::chrono::milliseconds(40));
  auto visible_before = area->is_caret_visible();
  assert(visible_before);
  (void)take(); // the rate change repaints the caret cell: settle the model
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  screen.run_pending_timers(); // first tick: the caret leaves its "on" phase
  drain_events();
  auto blink_off = take();
  assert(not blink_off.empty() && "the blink tick must repaint the caret cell");
  assert(blink_off.size() * 4 < full_paint.size() && "a blink tick repaints the caret cell only");
  assert(model.apply(blink_off));

  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  screen.run_pending_timers(); // second tick: the caret is shown again
  drain_events();
  auto blink_on = take();
  assert(not blink_on.empty() && "the second blink tick must repaint the caret cell again");
  assert(blink_on.size() * 4 < full_paint.size() && "a blink tick repaints the caret cell only");
  assert(model.apply(blink_on));

  // Hiding the caret erases only its cell; showing it again repaints only
  // that cell (the same paint gate the keyboard focus uses).
  area->set_caret_blink_rate(std::chrono::milliseconds::zero());
  (void)take(); // the rate change repainted the caret cell: settle the model
  area->set_caret_visible(false);
  drain_events();
  auto hide_paint = take();
  assert(not hide_paint.empty() && "hiding the caret must repaint its cell");
  assert(hide_paint.size() * 4 < full_paint.size() && "hiding the caret repaints the cell only");
  assert(model.apply(hide_paint));
  area->set_caret_visible(true);
  drain_events();
  auto show_paint = take();
  assert(not show_paint.empty() && "showing the caret must repaint its cell");
  assert(show_paint.size() * 4 < full_paint.size() && "showing the caret repaints the cell only");
  assert(model.apply(show_paint));

  // The oracle: the incremental model must equal a full re-emission.
  dynamic_cast<TextScreen&>(screen).clear();
  screen.refresh();
  auto full_reemit = take();
  auto oracle = VtModel { dim.height, dim.width };
  assert(oracle.apply(full_reemit));
  for (auto row = 0; row < dim.height; ++row) {
    assert(oracle.cell[std::size_t(row)] == model.cell[std::size_t(row)] && "the incremental paints must reproduce the full-repaint image");
  }

  // Typing at the end of a line must keep the caret at that line's end: the
  // edit rewinds the buffer's lazy line index to the anchor before it, and
  // the caret geometry must not measure the caret's column through the
  // rewound index (line_start clamps beyond it to the content end, which
  // used to land the caret on column 0 of its row).
  {
    auto eol_buffer = TextBuffer::create_empty();
    auto eol_text = std::string { "aaa\nbbbbbbbb\nccc\n" };
    eol_buffer->replace(0, 0, eol_text);
    eol_buffer->scan_to_end();
    area->set_buffer(eol_buffer);
    drain_events();
    (void)take();

    // The end of line 1 (the widest line): its newline byte.
    area->set_caret(offset_of_line(eol_text, 2) - 1);
    drain_events();
    (void)take();
    type_char(frame, Char { 'z' });
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    auto eol_paint = take();
    auto eol_model = VtModel { dim.height, dim.width };
    assert(eol_model.apply(eol_paint));

    // File line 1 is screen row 2 (row 0 is the frame's top border, row 1 is
    // file line 0). The caret block must cover the cell right after
    // "bbbbbbbbz" (screen column 10), not the row's first content cell.
    auto caret_row = 2;
    auto caret_col = 10;
    assert(eol_model.cell[std::size_t(caret_row)][std::size_t(caret_col)] == " " && "the end-of-line caret is a blank cell");
    assert(eol_model.inverse[std::size_t(caret_row)][std::size_t(caret_col)] && "the caret must sit at the end of the typed line");
    assert(not eol_model.inverse[std::size_t(caret_row)][1] && "the caret must not be drawn at the start of its row");
    // The typed character belongs at the end of the line: cells 1..8 are the
    // original b's and cell 9 holds the inserted z. (Compared cell by cell:
    // row_text joins UTF-8 glyphs, so a byte offset would land inside the
    // multi-byte frame-border glyph at column 0.)
    auto eol_content = std::string { };
    for (auto x = 1; x <= 9; ++x) {
      eol_content += eol_model.cell[std::size_t(caret_row)][std::size_t(x)];
    }
    assert(eol_content == "bbbbbbbbz" && "the typed character belongs at the end of the line");
  }

  std::cout.rdbuf(old_cout);
  std::fprintf(stderr, "test_TextArea_caret: typing row paints %zu/%zu bytes, blink ticks %zu/%zu bytes, images identical\n",
      type_paint.size() + type2_paint.size(), full_paint.size(), blink_off.size() + blink_on.size(), full_paint.size());
}
