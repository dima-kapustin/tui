// Tests of the button focus underline: it is a keyboard focus indicator, so a
// button focused by an activating mouse event (a click) must not paint it,
// while keyboard traversal -- and the FocusPainted switch -- still do.
//
// A real Frame with two buttons is shown on the text screen and the emitted
// escape stream is captured and replayed onto a small VT model (cursor moves,
// SGR and the scroll operations, the same decoder the other screen tests
// use). Only the underline attribute (SGR 4/24) is tracked, cell by cell, and
// the checks are made on the cells that spell the button's label.

#include <tui++/BorderLayout.h>
#include <tui++/Button.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Point.h>
#include <tui++/Screen.h>
#include <tui++/event/FocusEvent.h>
#include <tui++/event/InvocationEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace tui;

namespace {

// ---------------------------------------------------------------------------
// a minimal VT decoder (same model as test_TextScreenScroll): cursor moves,
// SGR, DECSTBM and IL/DL. `underlined` records the underline the SGR state
// had when each cell was printed.

struct VtModel {
  int rows;
  int cols;
  // One glyph (a possibly multi-byte UTF-8 character) per cell, so the
  // model's column indices are cell indices like the escape positions.
  std::vector<std::vector<std::string>> cell;
  std::vector<std::vector<bool>> underlined;
  int cursor_row = 0;
  int cursor_col = 0;
  int region_top = 0;    // DECSTBM, 0-based inclusive
  int region_bottom = 0;

  explicit VtModel(int rows, int cols) :
      rows(rows), cols(cols), region_bottom(rows - 1),
      cell(std::size_t(rows), std::vector<std::string>(std::size_t(cols), " ")),
      underlined(std::size_t(rows), std::vector<bool>(std::size_t(cols), false)) {
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
    std::fill(this->underlined[std::size_t(y)].begin(),
        this->underlined[std::size_t(y)].end(), false);
  }

  // The SGR state the cells printed from now on carry. Only what the text
  // screen emits is interpreted: a reset, the attributes it uses, and the
  // truecolor foreground/background.
  std::string attributes;

  void set_sgr(std::vector<int> const &params) {
    auto i = std::size_t { 0 };
    auto apply_code = [&](int code) {
      if (code == 0) {
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
      }
    };

    while (i < params.size()) {
      auto code = params[i];
      if ((code == 38 or code == 48) and i + 4 < params.size() and params[i + 1] == 2) {
        i += 5; // truecolor: no attribute information
      } else {
        apply_code(code);
        ++i;
      }
    }
  }

  void print(std::string const &glyph) {
    auto underlined = this->attributes.find('u') != std::string::npos;
    this->cell[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = glyph;
    this->underlined[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = underlined;
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
      this->underlined[std::size_t(y)] = this->underlined[std::size_t(y + n)];
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
      this->underlined[std::size_t(y)] = this->underlined[std::size_t(y - n)];
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

// Pops and dispatches the queued repaint invocations (the only events the
// test itself generates); anything else is ignored the way the screen tests
// always have been (no windows' listeners need them for painting).
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

// The top-left cell of the first row that spells `label` in consecutive
// cells. The labels are single-cell ASCII glyphs, so one glyph is one cell.
struct LabelCells {
  int row;
  int column;
};

std::optional<LabelCells> find_label(VtModel const &model, std::string const &label) {
  for (auto y = 0; y < model.rows; ++y) {
    for (auto x = 0; x + int(label.size()) <= model.cols; ++x) {
      auto match = true;
      for (auto i = std::size_t { 0 }; i < label.size(); ++i) {
        auto const &cell = model.cell[std::size_t(y)][std::size_t(x) + i];
        if (cell != std::string(1, label[i])) {
          match = false;
          break;
        }
      }
      if (match) {
        return LabelCells { y, x };
      }
    }
  }
  return std::nullopt;
}

}

void test_ButtonFocus() {
  std::fprintf(stderr, "test_ButtonFocus: the focus underline is a keyboard indicator\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());

  auto dim = screen.get_size();
  assert(dim.width > 20 and dim.height > 6);

  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_name("button focus test frame");

  // Two buttons: the subject under test, and a sink that takes the focus away
  // between the phases (a focus request for the button that already owns it
  // is a no-op and would leave the previous cause in place).
  auto content = frame->get_content_pane();
  auto subject = make_component<Button>("Focus"); // no mnemonic: only the focus underlines it
  auto sink = make_component<Button>("Sink");
  content->add(subject, BorderLayout::NORTH);
  content->add(sink, BorderLayout::SOUTH);
  frame->set_visible(true);
  drain_events();

  // A full re-emission of the frame, decoded: TextScreen::clear drops the
  // "already on the terminal" shadow, so the refresh emits every cell.
  auto reemit = [&] {
    drain_events();
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    auto bytes = capture.str();
    capture.str({ });
    auto model = VtModel { dim.height, dim.width };
    assert(model.apply(bytes) && "the text screen's emission must decode");
    return model;
  };

  // How many of the label's cells the emission underlined (all of them, or
  // none: the label is painted as one run).
  auto label_underlines = [&](VtModel const &model) {
    auto const label = std::string { "Focus" };
    auto cells = find_label(model, label);
    assert(cells.has_value() && "the button's label must be on the screen");
    auto count = 0;
    for (auto i = std::size_t { 0 }; i < label.size(); ++i) {
      if (model.underlined[std::size_t(cells->row)][std::size_t(cells->column) + i]) {
        ++count;
      }
    }
    return count;
  };

  auto label_length = int(std::string { "Focus" }.size());

  // A click's focus is not a keyboard focus: the label stays plain.
  sink->request_focus(FocusEvent::Cause::TRAVERSAL_FORWARD);
  drain_events();
  subject->request_focus(FocusEvent::Cause::MOUSE_EVENT);
  assert(subject->is_focus_owner() && "the button must take the requested focus");
  auto mouse_focused = reemit();
  assert(label_underlines(mouse_focused) == 0 && "a mouse-focused button must not underline its label");

  // Keyboard traversal keeps painting the indicator.
  sink->request_focus(FocusEvent::Cause::TRAVERSAL_FORWARD);
  drain_events();
  subject->request_focus(FocusEvent::Cause::TRAVERSAL_FORWARD);
  assert(subject->is_focus_owner() && "the button must take the traversed focus");
  auto traversal_focused = reemit();
  assert(label_underlines(traversal_focused) == label_length && "a keyboard-focused button must underline its label");

  // The FocusPainted switch still suppresses it.
  subject->set_focus_painted(false);
  auto unpainted = reemit();
  assert(label_underlines(unpainted) == 0 && "set_focus_painted(false) must suppress the underline");
  subject->set_focus_painted(true);

  // A real mouse press focuses through the button's own delegate, whose
  // request carries the mouse cause.
  sink->request_focus(FocusEvent::Cause::TRAVERSAL_FORWARD);
  drain_events();
  auto at = subject->get_location_on_screen();
  auto local = convert_point_from_screen(Point { at.x + subject->get_width() / 2, at.y + subject->get_height() / 2 }, frame);
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MouseEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, local.x, local.y, false);
  frame->dispatch_event(*screen.get_event_queue().pop());
  drain_events();
  assert(subject->is_focus_owner() && "the press must put the keyboard focus on the button");
  auto clicked = reemit();
  assert(label_underlines(clicked) == 0 && "a clicked button must not underline its label");

  frame->set_visible(false);
  drain_events();
  std::cout.rdbuf(old_cout);

  std::fprintf(stderr, "test_ButtonFocus: ok (keyboard focus underlines, click focus does not)\n");
}
