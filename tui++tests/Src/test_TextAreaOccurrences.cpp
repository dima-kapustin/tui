// Tests of the TextArea occurrence highlight: the matched text (the word
// around the caret, or the selection) and the cells the paint pass actually
// highlights.
//
// The occurrence-text tests are model checks on a bare area. The painted tests
// show a real window tree (Frame + ScrollPane + TextArea) on the text screen,
// capture the emitted escape stream and replay it onto a model that keeps one
// background color per cell -- the only state the occurrence highlight changes
// -- so they can say which cells carry the highlight, not just that it was
// emitted. The edit phase is the interesting one: typing rewrites the word
// under the caret, which changes the matched text, and with it the highlight
// of rows the edit itself never damaged.

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
#include <vector>

using namespace tui;

namespace {

// The background of an occurrence and of a selected cell, as the model stores
// them (the RGB parameters of the SGR sequences the text screen emits for
// them; to_terminal makes every program color a truecolor). Kept in step with
// TextArea.cpp.
constexpr char OCCURRENCE_BG[] = "70;56;26";
constexpr char SELECTION_BG[] = "44;62;102";

// ---------------------------------------------------------------------------
// a minimal VT decoder that keeps one background color per cell

struct BgModel {
  int rows;
  int cols;
  std::vector<std::vector<std::string>> cell;
  std::vector<std::vector<std::string>> bg;
  int cursor_row = 0;
  int cursor_col = 0;
  int region_top = 0;
  int region_bottom = 0;
  std::string current = "-";

  explicit BgModel(int rows, int cols) :
      rows(rows), cols(cols), region_bottom(rows - 1),
      cell(std::size_t(rows), std::vector<std::string>(std::size_t(cols), " ")),
      bg(std::size_t(rows), std::vector<std::string>(std::size_t(cols), "-")) {
  }

  std::string row_text(int y) const {
    std::string text;
    for (auto const &glyph : this->cell[std::size_t(y)]) {
      text += glyph;
    }
    return text;
  }

  void print(std::string const &glyph) {
    this->cell[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = glyph;
    this->bg[std::size_t(this->cursor_row)][std::size_t(this->cursor_col)] = this->current;
    if (++this->cursor_col >= this->cols) {
      this->cursor_col = 0;
    }
  }

  void blank_row(int y) {
    this->cell[std::size_t(y)].assign(std::size_t(this->cols), " ");
    this->bg[std::size_t(y)].assign(std::size_t(this->cols), "-");
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
      std::swap(this->cell[std::size_t(y)], this->cell[std::size_t(y + n)]);
      std::swap(this->bg[std::size_t(y)], this->bg[std::size_t(y + n)]);
    }
    for (auto y = this->region_bottom - n + 1; y <= this->region_bottom; ++y) {
      blank_row(y);
    }
  }

  void insert_lines(int n) {
    auto first = std::clamp(this->cursor_row, this->region_top, this->region_bottom);
    n = std::clamp(n, 0, this->region_bottom - first + 1);
    for (auto y = this->region_bottom; y >= first + n; --y) {
      std::swap(this->cell[std::size_t(y)], this->cell[std::size_t(y - n)]);
      std::swap(this->bg[std::size_t(y)], this->bg[std::size_t(y - n)]);
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

      case State::CSI:
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
            // Only the background is tracked; the foreground and the
            // attributes (the caret's inverse) do not move it.
            auto at = std::size_t { 0 };
            while (at < params.size()) {
              auto code = params[at];
              if (code == 0 or code == 39 or code == 49) {
                this->current = "-";
                ++at;
              } else if ((code == 38 or code == 48) and at + 4 < params.size() and params[at + 1] == 2) {
                if (code == 48) {
                  this->current = std::to_string(params[at + 2]) + ";" + std::to_string(params[at + 3]) + ";" + std::to_string(params[at + 4]);
                }
                at += 5;
              } else if (code == 48 and at + 2 < params.size() and params[at + 1] == 5) {
                this->current = std::to_string(params[at + 2]);
                at += 3;
              } else {
                ++at;
              }
            }
            break;
          }
          case 'H':
          case 'f':
            this->cursor_row = std::clamp((params.size() > 0 and params[0] > 0 ? params[0] : 1) - 1, 0, this->rows - 1);
            this->cursor_col = std::clamp((params.size() > 1 and params[1] > 0 ? params[1] : 1) - 1, 0, this->cols - 1);
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
          case 'h':
          case 'l':
          case '?':
            break;
          default:
            return false; // an emission this model does not know: fail loudly
          }
          state = State::TEXT;
        }
        ++i;
        break;
      }
    }
    return true;
  }
};

// Dispatches the queued repaint invocations (the only events the test itself
// generates).
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

std::shared_ptr<TextArea> make_area(std::string const &content) {
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, content);
  buffer->scan_to_end();
  auto area = make_component<TextArea>();
  area->set_buffer(buffer);
  return area;
}

// The word the highlight matches for a caret at `offset`.
std::string word_at(std::shared_ptr<TextArea> const &area, std::uint64_t offset) {
  area->set_caret(offset);
  return area->get_occurrence_text();
}

// The first row whose text contains `text`, or -1.
int row_of(BgModel const &model, std::string const &text) {
  for (auto y = 0; y < model.rows; ++y) {
    if (model.row_text(y).find(text) != std::string::npos) {
      return y;
    }
  }
  return -1;
}

// How many cells the `word` occurrences of `row` carry `color`. The scan is
// cell by cell (a row's text is UTF-8 -- the frame's border is one multi-byte
// glyph -- so a byte offset in the text is not a cell index); occurrences are
// taken at every position where the word matches, which is what the paint pass
// highlights.
int colored_cells(BgModel const &model, int row, std::string const &word, std::string const &color) {
  auto cells = 0;
  for (auto x = 0; x + int(word.size()) <= model.cols; ++x) {
    auto text = std::string { };
    for (auto i = std::size_t { 0 }; i < word.size(); ++i) {
      text += model.cell[std::size_t(row)][std::size_t(x) + i];
    }
    if (text != word) {
      continue;
    }
    for (auto i = std::size_t { 0 }; i < word.size(); ++i) {
      if (model.bg[std::size_t(row)][std::size_t(x) + i] == color) {
        ++cells;
      }
    }
  }
  return cells;
}

// A shown Frame + ScrollPane + TextArea, with the key plumbing of a focused
// area: the matched text of a selection is only reachable through the keyboard
// commands (the area has no programmatic select()).
struct Harness {
  std::shared_ptr<Frame> frame;
  std::shared_ptr<TextArea> area;

  Harness(std::string const &content, bool readonly) {
    auto buffer = TextBuffer::create_empty();
    buffer->replace(0, 0, content);
    buffer->scan_to_end();

    auto pane = make_component<ScrollPane>();
    pane->set_name("occurrence test pane");
    auto area = make_component<TextArea>();
    area->set_name("occurrence test area");
    area->set_buffer(buffer);
    area->set_readonly(readonly);
    // A steady caret: the blink would repaint its cell on its own schedule.
    area->set_caret_blink_rate(std::chrono::milliseconds::zero());
    pane->set_viewport_view(area);

    auto frame = make_component<Frame>();
    frame->set_size(screen.get_size());
    frame->set_name("occurrence test frame");
    frame->add(pane);
    frame->set_visible(true);
    area->request_input_focus();
    drain_events();

    this->frame = frame;
    this->area = area;
  }

  // Dispatches one key event through the window, the way the terminal does.
  void type_key(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) {
    drain_events();
    screen.post<KeyEvent>(this->frame, KeyEvent::KEY_PRESSED, key_code, modifiers);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }

  void type_char(Char const &character) {
    drain_events();
    screen.post<KeyEvent>(this->frame, character, InputEvent::NO_MODIFIERS);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }

  // Shift+Right `count` times from the caret: a byte-range selection.
  void extend_right(int count) {
    for (auto i = 0; i < count; ++i) {
      type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN);
    }
  }
};

void test_occurrence_from_caret_word() {
  auto area = make_area("one two three\nalpha beta\n");
  assert(not area->is_occurrence_highlight());
  assert(area->get_occurrence_text().empty() && "the feature is off by default");

  area->set_occurrence_highlight(true);
  assert(area->is_occurrence_highlight());

  // The word around the caret, from any spot inside it (1), at its end (3, on
  // the separator) and at its start (4, on the first letter).
  assert(word_at(area, 0) == "one");
  assert(word_at(area, 1) == "one");
  assert(word_at(area, 3) == "one");
  assert(word_at(area, 4) == "two");
  assert(word_at(area, 7) == "two");
  assert(word_at(area, 8) == "three");
  assert(word_at(area, 13) == "three"); // on the line break
  assert(word_at(area, 14) == "alpha");
  assert(word_at(area, 19) == "alpha"); // the space after "alpha"
  assert(word_at(area, 20) == "beta");

  // A caret on a separator takes the word ending there; on a spot with no word
  // character on either side there is nothing to highlight.
  auto spaced = make_area("a  \n  b\n");
  spaced->set_occurrence_highlight(true);
  assert(word_at(spaced, 0) == "a");
  assert(word_at(spaced, 1) == "a");
  assert(word_at(spaced, 2).empty() && "a separator two bytes away is not the word");
  assert(word_at(spaced, 4).empty());
  auto between = make_area(" \n \n");
  between->set_occurrence_highlight(true);
  assert(word_at(between, 0).empty());
  assert(word_at(between, 2).empty());
  assert(word_at(between, 4).empty());

  // The content end holds no word (the trailing newline opens an empty line).
  auto trailing = make_area("word\n");
  trailing->set_occurrence_highlight(true);
  assert(word_at(trailing, 3) == "word");
  assert(word_at(trailing, 4) == "word"); // the newline: the word ends there
  assert(word_at(trailing, 5).empty() && "the empty last line holds no word");

  // Multi-byte characters are whole words, and a caret inside one (offsets are
  // bytes) still finds the word that contains it.
  auto unicode = make_area("caf\xC3\xA9 x\n");
  unicode->set_occurrence_highlight(true);
  assert(word_at(unicode, 1) == "caf\xC3\xA9");
  assert(word_at(unicode, 3) == "caf\xC3\xA9"); // the é's first byte
  assert(word_at(unicode, 4) == "caf\xC3\xA9"); // inside the é
  assert(word_at(unicode, 5) == "caf\xC3\xA9"); // after it, on the separator
  assert(word_at(unicode, 6) == "x");           // the separator does not glue words
  assert(word_at(unicode, 7) == "x");           // the line break, i.e. the word's end
  assert(word_at(unicode, 8).empty());          // the empty last line

  // A run longer than the cap is no pattern: the scan stays bounded, which is
  // what keeps the highlight O(screen) instead of O(file). A run of exactly
  // the cap that ends there is still a word.
  auto long_word = make_area(std::string(TextArea::MAX_OCCURRENCE_TEXT + 1, 'a') + " tail\n");
  long_word->set_occurrence_highlight(true);
  assert(word_at(long_word, 0).empty() && "a run longer than the cap is not a match");
  auto exact = make_area(std::string(TextArea::MAX_OCCURRENCE_TEXT, 'a') + " tail\n");
  exact->set_occurrence_highlight(true);
  assert(word_at(exact, 10) == std::string(TextArea::MAX_OCCURRENCE_TEXT, 'a'));
  assert(word_at(exact, 10).size() == TextArea::MAX_OCCURRENCE_TEXT);
  auto at_end = make_area(std::string(TextArea::MAX_OCCURRENCE_TEXT, 'a'));
  at_end->set_occurrence_highlight(true);
  assert(word_at(at_end, TextArea::MAX_OCCURRENCE_TEXT) == std::string(TextArea::MAX_OCCURRENCE_TEXT, 'a'));

  // Turning the feature off drops the match.
  area->set_occurrence_highlight(false);
  assert(word_at(area, 1).empty());
}

void test_occurrence_from_selection() {
  // The last line is longer than the match cap, so select-all is ignored.
  auto harness = Harness { "alpha beta alpha\nbeta\n" + std::string(200, 'x') + "\n", false };
  auto area = harness.area;
  area->set_occurrence_highlight(true);

  // The selection is the pattern, exact bytes and all -- including a run of
  // separators, which the caret's word would never produce.
  area->set_caret(0);
  harness.extend_right(5);
  assert(area->get_occurrence_text() == "alpha");
  area->set_caret(5);
  harness.extend_right(2);
  assert(area->get_occurrence_text() == " b");

  // A selection longer than the cap is ignored rather than read: Ctrl+A on a
  // huge file must not read it to build the match.
  area->select_all();
  assert(area->get_occurrence_text().empty());

  // A column selection has no single byte text: the caret corner's word is
  // used. Two Alt+Shift steps right and one down make a block from (row 0,
  // cell 0) to (row 1, cell 2), whose caret is inside row 1's "beta".
  area->set_caret(0);
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  assert(area->is_block_selection());
  assert(area->get_occurrence_text() == "beta");

  // A plain move collapses the selection: the word under the caret is the
  // pattern again.
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::NO_MODIFIERS);
  assert(not area->has_selection());
  assert(area->get_occurrence_text() == "beta");

  // Turning the feature off drops the match without touching the selection.
  area->set_caret(0);
  harness.extend_right(3);
  assert(area->has_selection());
  area->set_occurrence_highlight(false);
  assert(area->get_occurrence_text().empty());
  assert(area->has_selection());
  auto selection = area->get_selection();
  assert(selection.first == 0 and selection.second == 3);
}

// The painted highlight of a read-only view: the off/on states, a caret move
// onto another word, and a selection.
void test_painted_highlight_view() {
  std::fprintf(stderr, "test_TextArea_occurrences: the view highlight on the text screen\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });

  auto take = [&] {
    auto bytes = capture.str();
    capture.str({ });
    return bytes;
  };

  auto harness = Harness { "alpha beta alpha\nbeta alpha beta\nalpha gamma\n", true };
  auto area = harness.area;
  auto dim = screen.get_size();

  // A full re-emission of the frame, decoded into a fresh model: every phase
  // is checked against the whole screen, so a stale cell is caught wherever it
  // is.
  auto paint = [&] {
    drain_events();
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    auto model = BgModel { dim.height, dim.width };
    assert(model.apply(take()));
    return model;
  };

  auto baseline = paint();
  auto row1 = row_of(baseline, "alpha beta alpha");
  auto row2 = row_of(baseline, "beta alpha beta");
  auto row3 = row_of(baseline, "alpha gamma");
  assert(row1 >= 0 and row2 >= 0 and row3 >= 0);
  assert(colored_cells(baseline, row1, "alpha", OCCURRENCE_BG) == 0 && "off: no cell is highlighted");

  // On, with the caret on the space ending the first "alpha": every visible
  // occurrence is highlighted -- on the caret's row and on the rows the caret
  // did not touch.
  area->set_caret(5);
  area->set_occurrence_highlight(true);
  auto on = paint();
  assert(colored_cells(on, row1, "alpha", OCCURRENCE_BG) == 10 && "both occurrences on the caret's row");
  assert(colored_cells(on, row2, "alpha", OCCURRENCE_BG) == 5 && "an occurrence on another row");
  assert(colored_cells(on, row3, "alpha", OCCURRENCE_BG) == 5 && "an occurrence on the last row");
  assert(colored_cells(on, row1, "beta", OCCURRENCE_BG) == 0 && "only the match is highlighted");

  // The caret moves into another word: the match follows it, and the rows the
  // caret never entered are repainted with the new highlight (and without the
  // old one).
  area->set_caret(10); // the space ending the first "beta"
  assert(area->get_occurrence_text() == "beta");
  auto moved = paint();
  assert(colored_cells(moved, row1, "beta", OCCURRENCE_BG) == 4);
  assert(colored_cells(moved, row2, "beta", OCCURRENCE_BG) == 8);
  assert(colored_cells(moved, row1, "alpha", OCCURRENCE_BG) == 0 && "the old match's cells are cleared");
  assert(colored_cells(moved, row2, "alpha", OCCURRENCE_BG) == 0);
  assert(colored_cells(moved, row3, "alpha", OCCURRENCE_BG) == 0);

  // A selection is the match: the selected occurrence keeps the selection
  // color (it wins over the highlight), and the other occurrences -- on other
  // rows -- turn highlighted.
  area->set_caret(0);
  harness.extend_right(5); // "alpha"
  assert(area->get_occurrence_text() == "alpha");
  auto selected = paint();
  assert(colored_cells(selected, row1, "alpha", SELECTION_BG) == 5 && "the selected occurrence");
  assert(colored_cells(selected, row1, "alpha", OCCURRENCE_BG) == 5 && "the other occurrence on the row");
  assert(colored_cells(selected, row2, "alpha", OCCURRENCE_BG) == 5);
  assert(colored_cells(selected, row3, "alpha", OCCURRENCE_BG) == 5);

  // Off again: the whole screen loses the highlight.
  area->set_occurrence_highlight(false);
  auto off = paint();
  assert(colored_cells(off, row1, "alpha", OCCURRENCE_BG) == 0);
  assert(colored_cells(off, row2, "alpha", OCCURRENCE_BG) == 0);
  assert(colored_cells(off, row3, "alpha", OCCURRENCE_BG) == 0);

  std::cout.rdbuf(old_cout);
}

// The painted highlight while editing: typing rewrites the word under the
// caret, so the match changes with a keystroke and the highlight of rows the
// edit never damaged must follow.
void test_painted_highlight_edit() {
  std::fprintf(stderr, "test_TextArea_occurrences: the edit highlight on the text screen\n");

  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });

  auto take = [&] {
    auto bytes = capture.str();
    capture.str({ });
    return bytes;
  };

  auto harness = Harness { "ca\nx\ncat\n", false };
  auto area = harness.area;
  auto dim = screen.get_size();
  auto paint = [&] {
    drain_events();
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    auto model = BgModel { dim.height, dim.width };
    assert(model.apply(take()));
    return model;
  };

  // The match is "ca" (the caret's word): it occurs as the whole first line
  // and as the head of the third line's "cat" -- a partial-word match.
  area->set_caret(2);
  area->set_occurrence_highlight(true);
  auto before = paint();
  auto row1 = row_of(before, "ca");
  auto row3 = row_of(before, "cat");
  assert(row1 >= 0 and row3 >= 0 and row1 != row3);
  assert(colored_cells(before, row1, "ca", OCCURRENCE_BG) == 2);
  assert(colored_cells(before, row3, "ca", OCCURRENCE_BG) == 2 && "the head of a longer word matches too");

  // Typing 't' turns the caret's word into "cat": the edited row is repainted
  // by the edit itself, but the third row only changes because the match
  // changed -- its highlight must grow to the whole word.
  harness.type_char(Char { 't' });
  assert(area->get_occurrence_text() == "cat");
  auto after = paint();
  assert(colored_cells(after, row1, "cat", OCCURRENCE_BG) == 3);
  assert(colored_cells(after, row3, "cat", OCCURRENCE_BG) == 3 && "the rows the edit did not damage follow the new match");

  std::cout.rdbuf(old_cout);
}

} // namespace

void test_TextArea_occurrences() {
  test_occurrence_from_caret_word();
  test_occurrence_from_selection();
  test_painted_highlight_view();
  test_painted_highlight_edit();
  std::fprintf(stderr, "test_TextArea_occurrences: ok\n");
}
