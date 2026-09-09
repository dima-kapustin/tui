// Tests of the TextAreaDemo find-all results panel wiring: the find-all
// results pane (a hidden ScrollPane in a BorderLayout bottom strip) opening
// over the file view, the read-only results TextArea listing one hit per
// row, and the mouse/keyboard preview flow (click a row or walk it with the
// caret keys to jump the file view, Enter previews and closes, Esc closes,
// F9 re-runs). The demo's logic is replicated here (it lives in
// TextAreaDemo/src/main.cpp, which no test target links), so this guards the
// framework behaviors the demo leans on: a window with TWO TextAreas routes
// keys to the focus owner through its forwarders while window listeners run
// after the area's own handling; a ScrollPane hidden in a BorderLayout region
// takes no space and re-lays out when shown; set_buffer on the view of a
// shown ScrollPane resizes the view; a read-only area swallows typing and
// Enter without touching its buffer.
//
// Like test_TextAreaSelection it drives real events (keys to the window,
// presses to the click target) through a Frame + two ScrollPane/TextArea
// trees and checks the public component state.

#include <tui++/BorderLayout.h>
#include <tui++/Char.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Panel.h>
#include <tui++/Screen.h>
#include <tui++/ScrollPane.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/event/InvocationEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace tui;

namespace {

constexpr std::size_t FIND_ALL_LIMIT = 2000;
constexpr std::size_t RESULT_ROWS = 10;

// Dispatches the queued repaint invocations (the only events the test itself
// generates); anything else is ignored.
void drain_events() {
  auto &queue = screen.get_event_queue();
  for (auto i = 0; i < 4000; ++i) {
    auto event = queue.pop(std::chrono::milliseconds(2));
    if (not event) {
      return;
    }
    if (event->id == InvocationEvent::INVOCATION) {
      static_cast<InvocationEvent &>(*event).dispatch();
    }
  }
  assert(false && "the event queue must drain");
}

// The contents of the whole buffer, for row-text assertions.
std::string content_of(TextArea const &area) {
  auto buffer = area.get_buffer();
  return buffer->read(0, buffer->length());
}

// The demo's hit list and helpers, kept 1:1 with TextAreaDemo/src/main.cpp.
struct FindHit {
  std::uint64_t offset;
  std::uint64_t line;
  std::uint64_t column;
};

struct Harness {
  std::shared_ptr<Frame> frame;
  std::shared_ptr<ScrollPane> pane;
  std::shared_ptr<TextArea> area;
  std::shared_ptr<Panel> bottom;
  std::shared_ptr<ScrollPane> results_pane;
  std::shared_ptr<TextArea> results;
  std::vector<FindHit> hits;
  bool capped = false;
  std::size_t previewed = SIZE_MAX;

  explicit Harness(std::string const &content) {
    auto buffer = TextBuffer::create_empty();
    buffer->replace(0, 0, content);

    auto frame = make_component<Frame>();
    frame->set_size(screen.get_size());
    frame->set_name("find all test frame");
    auto pane = make_component<ScrollPane>();
    pane->set_name("find all file pane");
    auto area = make_component<TextArea>();
    area->set_name("find all file area");
    area->set_buffer(buffer);
    area->set_caret_blink_rate(std::chrono::milliseconds::zero());
    pane->set_viewport_view(area);
    frame->add(pane);

    // The results pane in a bottom strip (BorderLayout honors the strip's
    // preferred height, so the pane takes no space while hidden).
    auto bottom = make_component<Panel>();
    bottom->set_layout(std::make_shared<BorderLayout>());
    auto results_pane = make_component<ScrollPane>();
    results_pane->set_preferred_size(Dimension { 0, int(RESULT_ROWS) });
    results_pane->set_visible(false);
    auto results = make_component<TextArea>();
    results->set_name("find all results");
    results->set_readonly(true);
    results->set_line_wrap(true);
    results->set_caret_blink_rate(std::chrono::milliseconds::zero());
    results->set_buffer(TextBuffer::create_empty());
    results_pane->set_viewport_view(results);
    bottom->add(results_pane);
    frame->add(bottom, BorderLayout::SOUTH);

    frame->set_visible(true);
    // The demo's click handler: preview the clicked row (the area's own
    // press handler, registered earlier, already placed the caret there).
    results->add_listener([this](MousePressEvent &e) {
      if (e.id == MousePressEvent::MOUSE_PRESSED) {
        preview(std::size_t(std::max(0, e.y)));
      }
    });
    // The demo's window key handler, registered after the areas' key
    // forwarders (which add themselves at show time): F9 runs the search;
    // while the results area owns the focus its caret has already moved
    // under the key, so the row it landed on is previewed; Enter previews
    // and closes the panel; Esc closes it from either text area.
    frame->add_listener([this](KeyEvent &e) {
      if (e.id != KeyEvent::KEY_PRESSED) {
        return;
      }
      auto open = this->results_pane->is_visible();
      if (e.get_key_code() == KeyEvent::VK_F9) {
        run_find_all();
        e.consume();
        return;
      }
      if (not open) {
        return;
      }
      auto row = std::size_t(this->results->get_buffer()->offset_to_line(this->results->get_caret()).first);
      if (not this->results->is_focus_owner()) {
        if (e.get_key_code() == KeyEvent::VK_ESCAPE) {
          close_results();
        }
        return;
      }
      switch (e.get_key_code()) {
      case KeyEvent::VK_UP:
      case KeyEvent::VK_DOWN:
      case KeyEvent::VK_LEFT:
      case KeyEvent::VK_RIGHT:
      case KeyEvent::VK_PAGE_UP:
      case KeyEvent::VK_PAGE_DOWN:
      case KeyEvent::VK_HOME:
      case KeyEvent::VK_END:
        preview(row);
        break;
      case KeyEvent::VK_ENTER:
        preview(row);
        close_results();
        break;
      case KeyEvent::VK_ESCAPE:
        close_results();
        break;
      default:
        break;
      }
    });

    area->request_input_focus();
    drain_events();

    this->frame = frame;
    this->pane = pane;
    this->area = area;
    this->bottom = bottom;
    this->results_pane = results_pane;
    this->results = results;
  }

  void preview(std::size_t index) {
    if (index >= this->hits.size() or index == this->previewed) {
      return;
    }
    this->previewed = index;
    this->area->set_caret(this->hits[index].offset);
  }

  void close_results() {
    if (this->results_pane->is_visible()) {
      this->results_pane->set_visible(false);
      this->results_pane->revalidate();
    }
    this->area->request_input_focus();
  }

  void run_find_all() {
    auto pattern = this->area->get_search_pattern();
    if (pattern.empty()) {
      if (not this->area->is_search_mode()) {
        this->area->show_message("no pattern");
      }
      return;
    }
    if (this->area->is_search_mode()) {
      this->area->close_search_entry();
    }
    auto source = this->area->get_buffer();
    auto offsets = source->find_all(pattern, 0, FIND_ALL_LIMIT + 1);
    this->capped = offsets.size() > FIND_ALL_LIMIT;
    if (this->capped) {
      offsets.resize(FIND_ALL_LIMIT);
    }
    this->hits.clear();
    this->hits.reserve(offsets.size());
    auto rows = std::string();
    rows.reserve(offsets.size() * 96);
    for (auto offset : offsets) {
      source->ensure_scanned_to(offset);
      auto [line, column] = source->offset_to_line(offset);
      this->hits.push_back({ offset, line, column });
      char head[32];
      std::snprintf(head, sizeof head, "%10llu:%-6llu ",
                    static_cast<unsigned long long>(line),
                    static_cast<unsigned long long>(column));
      rows += head;
      auto ranges = source->read_line_ranges(line, 1);
      if (not ranges.empty()) {
        // end points at the terminating '\n', so the content length is
        // end - start already.
        auto text_len = ranges[0].end - ranges[0].start;
        if (text_len > 0) {
          rows += source->read(ranges[0].start, std::min<std::uint64_t>(text_len, 204));
        }
      }
      rows += '\n';
    }
    if (offsets.empty()) {
      rows += "(no matches)\n";
    } else if (this->capped) {
      rows += "(more hits than the limit: refine the pattern)\n";
    }
    auto results_buffer = TextBuffer::create_empty();
    results_buffer->replace(0, 0, rows);
    this->results->set_buffer(results_buffer);
    this->previewed = SIZE_MAX;
    if (not this->results_pane->is_visible()) {
      this->results_pane->set_visible(true);
    }
    this->results_pane->revalidate();
    this->results_pane->get_viewport()->set_view_position(0, 0);
    this->results->request_input_focus();
  }

  // One key through the window, the way the terminal does. Pending repaint
  // invocations queued by earlier calls (e.g. set_caret) are drained first:
  // otherwise the pop() below would return the stale invocation and the key
  // under test would be dropped with the post-dispatch drain.
  void type_key(KeyEvent::Type type, KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) {
    drain_events();
    screen.post<KeyEvent>(this->frame, type, key_code, modifiers);
    auto event = screen.get_event_queue().pop();
    assert(event != nullptr);
    this->frame->dispatch_event(*event);
    drain_events();
  }

  void type_key(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) {
    type_key(KeyEvent::KEY_PRESSED, key_code, modifiers);
  }

  void type_char(Char const &character) {
    drain_events();
    screen.post<KeyEvent>(this->frame, character, InputEvent::NO_MODIFIERS);
    auto event = screen.get_event_queue().pop();
    assert(event != nullptr);
    this->frame->dispatch_event(*event);
    drain_events();
  }

  // The frame-space coordinates of a results-list cell (x, y) (the initial
  // scroll is zero, so a list row is the view row).
  Point results_point_of(int x, int y) {
    auto px = x;
    auto py = y;
    for (auto component = std::shared_ptr<Component> { this->results }; component; component = component->get_parent()) {
      px += component->get_x();
      py += component->get_y();
    }
    return { px, py };
  }

  void press(Point const &at, InputEvent::Modifiers modifiers) {
    drain_events();
    screen.post<MousePressEvent>(this->frame, MousePressEvent::MOUSE_PRESSED, MouseEvent::LEFT_BUTTON, modifiers | InputEvent::LEFT_BUTTON_DOWN, at.x, at.y, false);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }

  void release(Point const &at) {
    drain_events();
    screen.post<MousePressEvent>(this->frame, MousePressEvent::MOUSE_RELEASED, MouseEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, at.x, at.y, false);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }
};

// The expected offsets of every non-overlapping occurrence, scanned straight
// from the document text we built (the same semantics as TextBuffer::find).
std::vector<std::uint64_t> expected_offsets(std::string const &text, std::string const &needle, std::size_t limit) {
  auto offsets = std::vector<std::uint64_t> { };
  auto pos = std::size_t { 0 };
  while (offsets.size() < limit) {
    pos = text.find(needle, pos);
    if (pos == std::string::npos) {
      break;
    }
    offsets.push_back(pos);
    pos += needle.size();
  }
  return offsets;
}

void test_find_all_panel() {
  std::fprintf(stderr, "test_TextArea_find_all: find-all results panel (open, preview, close, capped)\n");

  // Forty lines with one hit each, plus a repeated hit on line 39 so a line
  // can own several rows of the list.
  auto text = std::string { };
  for (auto i = 0; i < 40; ++i) {
    char line[96];
    std::snprintf(line, sizeof line, "alpha line %02d carries one needle here\n", i);
    text += line;
  }
  auto last = std::string { "alpha line 39 tail needle again\n" };
  text += last;

  auto harness = Harness { text };
  auto dim = screen.get_size();
  assert(dim.width >= 30 and dim.height >= 12);
  auto expected = expected_offsets(text, "needle", 41);
  assert(expected.size() == 41);
  auto full_pane_height = harness.pane->get_height();

  // The results strip is hidden: the file pane owns the whole height.
  assert(not harness.results_pane->is_visible());
  assert(harness.pane->get_height() == full_pane_height);

  // F3 opens the file area's search entry; the typed pattern feeds it.
  harness.type_key(KeyEvent::VK_F3, InputEvent::NO_MODIFIERS);
  assert(harness.area->is_search_mode());
  for (auto ch : std::string("needle")) {
    harness.type_char(Char(ch));
  }
  assert(harness.area->get_search_pattern() == "needle");

  // F9 runs the find-all and commits the entry: the strip opens (the file
  // pane gives up RESULT_ROWS rows), the results area owns the focus, and
  // every hit has a row whose text starts with the hit's line:column.
  harness.type_key(KeyEvent::VK_F9, InputEvent::NO_MODIFIERS);
  assert(harness.results_pane->is_visible());
  assert(harness.results->is_focus_owner());
  assert(not harness.area->is_search_mode());
  assert(harness.area->get_search_pattern() == "needle");
  assert(harness.pane->get_height() == full_pane_height - int(RESULT_ROWS));
  assert(harness.hits.size() == 41);
  assert(not harness.capped);
  for (auto i = std::size_t { 0 }; i < harness.hits.size(); ++i) {
    assert(harness.hits[i].offset == expected[i]);
  }
  auto list = content_of(*harness.results);
  // The row format: "<line right-aligned 10>:<column left-aligned 6> " then
  // the source line's text (the first hit is line 0, column 26 -- the
  // needle of "alpha line 00 carries one needle here").
  assert(list.find("         0:26     alpha line 00 carries one needle here") != std::string::npos);
  // The last loop line (line 39, column 26) and the extra line's hit (line
  // 40, column 19) follow.
  assert(list.find("        39:26     alpha line 39 carries one needle here") != std::string::npos);
  assert(list.find("        40:19     alpha line 39 tail needle again") != std::string::npos);
  // No match was previewed yet: the file caret is where it was.
  auto caret = harness.area->get_caret();
  assert(caret != expected[0]);

  // Down previews the second hit; the results caret moved to row 1 first.
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::NO_MODIFIERS);
  assert(harness.area->get_caret() == expected[1]);

  // A click on list row 3 previews its hit (and parks the results caret on
  // that row).
  auto at = harness.results_point_of(1, 3);
  harness.press(at, InputEvent::NO_MODIFIERS);
  harness.release(at);
  assert(harness.area->get_caret() == expected[3]);
  auto results_row = harness.results->get_buffer()->offset_to_line(harness.results->get_caret()).first;
  assert(results_row == 3);

  // Page down walks several hits in one step and previews the last one.
  harness.type_key(KeyEvent::VK_PAGE_DOWN, InputEvent::NO_MODIFIERS);
  auto walked = harness.results->get_buffer()->offset_to_line(harness.results->get_caret()).first;
  assert(walked > 3);
  assert(harness.area->get_caret() == expected[std::size_t(walked)]);

  // Enter previews the row under the caret and closes the panel; the file
  // area regains the focus, and the read-only results buffer survived the
  // Enter without a newline.
  auto before_close = content_of(*harness.results);
  harness.type_key(KeyEvent::VK_ENTER, InputEvent::NO_MODIFIERS);
  assert(harness.area->get_caret() == expected[std::size_t(walked)]);
  assert(not harness.results_pane->is_visible());
  assert(harness.area->is_focus_owner());
  assert(content_of(*harness.results) == before_close);
  assert(harness.pane->get_height() == full_pane_height);

  // F9 re-runs with the remembered pattern; Esc closes the panel again.
  harness.type_key(KeyEvent::VK_F9, InputEvent::NO_MODIFIERS);
  assert(harness.results_pane->is_visible());
  assert(harness.results->is_focus_owner());
  harness.type_key(KeyEvent::VK_ESCAPE, InputEvent::NO_MODIFIERS);
  assert(not harness.results_pane->is_visible());
  assert(harness.area->is_focus_owner());

  // Esc with the file area focused closes an open panel too.
  harness.type_key(KeyEvent::VK_F9, InputEvent::NO_MODIFIERS);
  harness.type_key(KeyEvent::VK_ESCAPE, InputEvent::NO_MODIFIERS);
  assert(not harness.results_pane->is_visible());

  // With no pattern the F9 keeps the panel closed and only reports it: F3
  // opens the search entry (clearing the remembered pattern), Esc closes it
  // empty, and F9 then has nothing to list.
  harness.type_key(KeyEvent::VK_F3, InputEvent::NO_MODIFIERS);
  harness.type_key(KeyEvent::VK_ESCAPE, InputEvent::NO_MODIFIERS);
  assert(harness.area->get_search_pattern().empty());
  harness.type_key(KeyEvent::VK_F9, InputEvent::NO_MODIFIERS);
  assert(not harness.results_pane->is_visible());
}

void test_find_all_capped() {
  std::fprintf(stderr, "test_TextArea_find_all: find-all cap and marker rows\n");

  // More hits than FIND_ALL_LIMIT, so the list cuts off and a marker row
  // (which must never preview) ends it.
  auto text = std::string { };
  for (auto i = 0; i < int(FIND_ALL_LIMIT) + 10; ++i) {
    text += "hit line " + std::to_string(i) + " with a hit\n";
  }
  auto harness = Harness { text };
  auto dim = screen.get_size();
  assert(dim.width >= 30 and dim.height >= 12);
  auto expected = expected_offsets(text, "hit", FIND_ALL_LIMIT);
  assert(expected.size() == FIND_ALL_LIMIT);

  harness.type_key(KeyEvent::VK_F3, InputEvent::NO_MODIFIERS);
  for (auto ch : std::string("hit")) {
    harness.type_char(Char(ch));
  }
  harness.type_key(KeyEvent::VK_F9, InputEvent::NO_MODIFIERS);
  assert(harness.results_pane->is_visible());
  assert(harness.capped);
  assert(harness.hits.size() == FIND_ALL_LIMIT);
  for (auto i = std::size_t { 0 }; i < harness.hits.size(); ++i) {
    assert(harness.hits[i].offset == expected[i]);
  }
  auto list = content_of(*harness.results);
  assert(list.find("(more hits than the limit: refine the pattern)") != std::string::npos);
  auto last_row_text = harness.results->get_buffer()->read_line_ranges(FIND_ALL_LIMIT, 1);
  assert(not last_row_text.empty());
  assert(last_row_text[0].end - last_row_text[0].start > 1);

  // The caret parked on the last hit (row FIND_ALL_LIMIT - 1) previews it;
  // Enter then closes. Walking onto the marker row (row FIND_ALL_LIMIT)
  // previews nothing.
  harness.area->set_caret(0);
  auto results_buffer = harness.results->get_buffer();
  // The results buffer's line index is lazy: index up to the last hit row
  // before resolving its start.
  results_buffer->ensure_line(FIND_ALL_LIMIT);
  auto last_hit_offset = results_buffer->line_start(FIND_ALL_LIMIT - 1);
  harness.results->set_caret(last_hit_offset);
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::NO_MODIFIERS);
  auto caret_line = harness.results->get_buffer()->offset_to_line(harness.results->get_caret()).first;
  assert(caret_line == FIND_ALL_LIMIT);
  auto caret = harness.area->get_caret();
  harness.type_key(KeyEvent::VK_ENTER, InputEvent::NO_MODIFIERS);
  assert(not harness.results_pane->is_visible());
  assert(harness.area->get_caret() == caret); // the marker row previewed nothing
}

} // namespace

void test_TextArea_find_all() {
  test_find_all_panel();
  test_find_all_capped();
  std::fprintf(stderr, "test_TextArea_find_all: ok\n");
}
