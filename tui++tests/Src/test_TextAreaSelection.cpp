// Tests of the TextArea mouse/keyboard selection: drags select arbitrary
// text, Alt+drag and Alt+Shift+arrows select column blocks, the column-select
// mode makes every selection gesture select columns, and the edit ops
// (copy/typing/delete, undo/redo) treat a column block as one unit.
//
// The key and mouse plumbing needs a shown window (keys go to windows, drags
// are retargeted to the press target by the window dispatcher and observed on
// the screen), so the tests drive real events through a Frame + ScrollPane +
// TextArea tree and check the public selection/clipboard state.

#include <tui++/Char.h>
#include <tui++/Clipboard.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Screen.h>
#include <tui++/ScrollPane.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/event/InvocationEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

using namespace tui;

namespace {

// Four rows of eight characters: even cells everywhere, so the column math in
// the assertions below stays simple.
const char *const ROWS[4] = { "abcdefgh", "ijklmnop", "qrstuvwx", "yz012345" };

std::string doc_text() {
  return std::string(ROWS[0]) + "\n" + ROWS[1] + "\n" + ROWS[2] + "\n" + ROWS[3] + "\n";
}

std::string content_of(TextArea const &area) {
  return area.get_buffer()->read(0, area.get_buffer()->length());
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

struct Harness {
  std::shared_ptr<Frame> frame;
  std::shared_ptr<TextArea> area;

  explicit Harness(std::string const &content) {
    auto buffer = TextBuffer::create_empty();
    buffer->replace(0, 0, content);
    buffer->scan_to_end();

    auto pane = make_component<ScrollPane>();
    pane->set_name("selection test pane");
    auto area = make_component<TextArea>();
    area->set_name("selection test area");
    area->set_buffer(buffer);
    area->set_caret_blink_rate(std::chrono::milliseconds::zero());
    pane->set_viewport_view(area);

    auto frame = make_component<Frame>();
    frame->set_size(screen.get_size());
    frame->set_name("selection test frame");
    frame->add(pane);
    frame->set_visible(true);
    area->request_input_focus();
    drain_events();

    this->frame = frame;
    this->area = area;
  }

  // Dispatches one key event through the window, the way the terminal does
  // (keys go to the focused window; the area receives them through its
  // forwarder). Pending repaint invocations queued by earlier calls (e.g.
  // set_caret) are drained first: otherwise the pop() below would return the
  // stale invocation, the key would be dropped with the post-dispatch drain,
  // and the event under test would silently never run.
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

  // The frame-space coordinates of area cell (x, y) (the view sits at
  // -view_position; the initial scroll is zero, so a click row is a file row).
  Point point_of(int x, int y) {
    auto px = x;
    auto py = y;
    for (auto component = std::shared_ptr<Component> { this->area }; component; component = component->get_parent()) {
      px += component->get_x();
      py += component->get_y();
    }
    return { px, py };
  }

  void press(Point const &at, InputEvent::Modifiers modifiers) {
    // The terminal reports a press with the button already down, so
    // was_button_down_before() treats it as just-released (a press reported
    // without it would be mistaken for a drag in progress).
    drain_events();
    screen.post<MousePressEvent>(this->frame, MousePressEvent::MOUSE_PRESSED, MouseEvent::LEFT_BUTTON, modifiers | InputEvent::LEFT_BUTTON_DOWN, at.x, at.y, false);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }

  void drag(Point const &to) {
    drain_events();
    screen.post<MouseDragEvent>(this->frame, MouseEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, to.x, to.y);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }

  void release(Point const &at) {
    drain_events();
    screen.post<MousePressEvent>(this->frame, MousePressEvent::MOUSE_RELEASED, MouseEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, at.x, at.y, false);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }

  // A full click gesture: the press/release pair the terminal reports, then
  // the MOUSE_CLICKED that carries the click count. The press comes first on
  // purpose -- the dispatcher only delivers a click to the component that
  // received the accompanying press.
  void click(Point const &at, unsigned click_count) {
    press(at, InputEvent::NO_MODIFIERS);
    release(at);
    drain_events();
    screen.post<MouseClickEvent>(this->frame, MouseEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, at.x, at.y, click_count, false);
    this->frame->dispatch_event(*screen.get_event_queue().pop());
    drain_events();
  }
};

void test_keyboard_column_block() {
  auto harness = Harness { doc_text() };
  auto area = harness.area;

  // Alt+Shift+Down x2 then Alt+Shift+Right x4 from the origin: a column
  // block over rows 0..2, cells 0..4 -- the first four characters of each.
  area->set_caret(0);
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  assert(area->has_selection());
  assert(area->is_block_selection());

  // Copy takes each row's covered cells, joined with newlines.
  area->copy();
  assert(Clipboard::get_text() == "abcd\nijkl\nqrst");

  // Typing replaces the whole block with the character, as one undo step.
  harness.type_char(Char('X'));
  assert(content_of(*area) == "Xefgh\nmnop\nuvwx\nyz012345\n");
  assert(not area->has_selection());

  area->undo();
  assert(content_of(*area) == doc_text());
  area->redo();
  assert(content_of(*area) == "Xefgh\nmnop\nuvwx\nyz012345\n");

  // Delete (forward) over the block removes its rows' bands; the tails close
  // up and one undo restores the whole block.
  area->undo(); // back to the untouched document
  assert(content_of(*area) == doc_text());
  area->set_caret(0);
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  assert(area->is_block_selection());
  harness.type_key(KeyEvent::VK_DELETE, InputEvent::NO_MODIFIERS);
  assert(content_of(*area) == "efgh\nmnop\nuvwx\nyz012345\n");
  assert(area->get_caret() == 0); // the block's top-left corner
  area->undo();
  assert(content_of(*area) == doc_text());
}

void test_column_select_mode() {
  auto harness = Harness { doc_text() };
  auto area = harness.area;

  // The mode makes plain Shift+arrows select columns (the escape hatch for
  // hosts where Alt+Shift never reaches the app).
  area->set_column_select_mode(true);
  area->set_caret(0);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN);
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::SHIFT_DOWN);
  assert(area->is_block_selection());
  area->copy();
  assert(Clipboard::get_text() == "abc\nijk");

  // A plain (non-extending) move collapses the block; with the mode off a
  // plain Shift+arrow selects a byte range again.
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::NO_MODIFIERS);
  assert(not area->has_selection());
  area->set_column_select_mode(false);
  area->set_caret(0);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::SHIFT_DOWN);
  assert(area->has_selection());
  assert(not area->is_block_selection());
  area->copy();
  assert(Clipboard::get_text() == "a");
}

void test_mouse_drag_selection() {
  auto harness = Harness { doc_text() };
  auto area = harness.area;
  auto dim = screen.get_size();
  assert(dim.width >= 30 and dim.height >= 12); // the pane shows rows 0..2

  // A plain press-drag-release from (1, 0) to (5, 2) selects the byte range
  // between the corners: row 0 from 'b', rows 1 and 2 up to the drag column.
  auto from = harness.point_of(1, 0);
  auto to = harness.point_of(5, 2);
  harness.press(from, InputEvent::NO_MODIFIERS);
  harness.drag(to);
  harness.release(to);
  assert(area->has_selection());
  assert(not area->is_block_selection());
  area->copy();
  assert(Clipboard::get_text() == "bcdefgh\nijklmnop\nqrstu");

  // A plain click collapses the selection again.
  harness.press(harness.point_of(3, 1), InputEvent::NO_MODIFIERS);
  harness.release(harness.point_of(3, 1));
  assert(not area->has_selection());

  // An Alt press-drag selects a column block between the corners (the mouse
  // reports encode Alt as Meta). Pressing at row 0 cell 2 and dragging to
  // row 2 cell 5 selects rows 0..2, cells 2..5 of each.
  harness.press(harness.point_of(2, 0), InputEvent::META_DOWN);
  harness.drag(harness.point_of(5, 2));
  harness.release(harness.point_of(5, 2));
  assert(area->has_selection());
  assert(area->is_block_selection());
  area->copy();
  assert(Clipboard::get_text() == "cde\nklm\nstu");
  auto block = area->get_block().value();
  assert(block.left == 2 and block.right == 5);
}

void test_ctrl_arrow_navigation() {
  // Sixty rows of "line 000" so paging has room to move (a page is the
  // viewport height minus one row).
  auto text = std::string { };
  for (auto i = 0; i < 60; ++i) {
    char line[32];
    std::snprintf(line, sizeof line, "line %03d\n", i);
    text += line;
  }
  auto harness = Harness { text };
  auto area = harness.area;

  // Park the caret mid-row on line 2 (each row is 9 bytes).
  auto line2 = std::uint64_t(2) * 9;
  area->set_caret(line2 + 3);

  // Ctrl+Left jumps to the start of the line, Ctrl+Right to its end (the
  // byte before the newline).
  harness.type_key(KeyEvent::VK_LEFT, InputEvent::CTRL_DOWN);
  assert(area->get_caret() == line2);
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::CTRL_DOWN);
  assert(area->get_caret() == line2 + 8);

  // Shift+Ctrl+Left selects from the line's start to the caret; the jump
  // itself extends the selection (the anchor stays at the line end).
  harness.type_key(KeyEvent::VK_LEFT, InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN);
  assert(area->has_selection());
  assert(not area->is_block_selection());
  area->copy();
  assert(Clipboard::get_text() == "line 002");

  // A plain move collapses again.
  harness.type_key(KeyEvent::VK_RIGHT, InputEvent::NO_MODIFIERS);
  assert(not area->has_selection());

  // Ctrl+Down pages down (a viewport-height step), Ctrl+Up pages back up.
  auto viewport = area->get_viewport();
  assert(viewport != nullptr);
  auto page = std::uint64_t(std::max(1, viewport->get_height() - 1));
  harness.type_key(KeyEvent::VK_DOWN, InputEvent::CTRL_DOWN);
  auto [down_line, down_col] = area->get_buffer()->offset_to_line(area->get_caret());
  assert(down_line == std::min<std::uint64_t>(2 + page, 59));
  harness.type_key(KeyEvent::VK_UP, InputEvent::CTRL_DOWN);
  auto [up_line, up_col] = area->get_buffer()->offset_to_line(area->get_caret());
  assert(up_line == 2);
  (void)down_col;
  (void)up_col;
}

void test_double_click_word() {
  // The classic click gestures of a text component: a double-click selects
  // the word (or the separator run) under the pointer, a triple-click the
  // line (see TextField::on_mouse_click).
  auto harness = Harness { "one two-three  four\nabc\n" };
  auto area = harness.area;
  auto dim = screen.get_size();
  assert(dim.width >= 24 and dim.height >= 6);

  // Inside "two" (row 0, cell 5).
  harness.click(harness.point_of(5, 0), 2);
  assert(area->has_selection());
  assert(not area->is_block_selection());
  area->copy();
  assert(Clipboard::get_text() == "two");

  // On a single separator and on a run of them: the run is selected.
  harness.click(harness.point_of(7, 0), 2);
  area->copy();
  assert(Clipboard::get_text() == "-");
  harness.click(harness.point_of(14, 0), 2);
  area->copy();
  assert(Clipboard::get_text() == "  " && "both spaces of the run");

  // Past the end of a line's text: the line's last word. (The view is one
  // cell wider than its widest line -- the end-of-line caret's cell -- so
  // cell 19 is the last one a click can land on here.)
  assert(area->get_width() == 20);
  harness.click(harness.point_of(19, 0), 2);
  area->copy();
  assert(Clipboard::get_text() == "four");

  // The word never reaches across the line break.
  harness.click(harness.point_of(1, 1), 2);
  area->copy();
  assert(Clipboard::get_text() == "abc");

  // A triple-click takes the whole line.
  harness.click(harness.point_of(3, 0), 3);
  area->copy();
  assert(Clipboard::get_text() == "one two-three  four");

  // A single click stays a caret placement.
  harness.click(harness.point_of(2, 0), 1);
  assert(not area->has_selection());
  assert(area->get_caret() == 2);

  // The word gesture is a text gesture, not a column one: the column-select
  // mode does not turn the double-click into a block.
  area->set_column_select_mode(true);
  harness.click(harness.point_of(5, 0), 2);
  assert(area->has_selection());
  assert(not area->is_block_selection());
  area->copy();
  assert(Clipboard::get_text() == "two");
  area->set_column_select_mode(false);
}

} // namespace

void test_TextArea_selection() {
  test_keyboard_column_block();
  test_column_select_mode();
  test_mouse_drag_selection();
  test_ctrl_arrow_navigation();
  test_double_click_word();
  std::fprintf(stderr, "test_TextArea_selection: ok\n");
}
