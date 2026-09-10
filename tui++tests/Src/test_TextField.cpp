// The TextField: Swing's JTextField. The test drives a real field inside a
// frame on the text screen and exercises the classic command set -- caret
// navigation (plain, word-wise, Home/End, with the Shift selection variants),
// typing and deletion, select-all, the clipboard actions and undo/redo, the
// Enter ActionEvent, the mouse gestures (click, drag, double-click) and the
// read-only mode.

#include <tui++/BorderLayout.h>
#include <tui++/Button.h>
#include <tui++/Clipboard.h>
#include <tui++/Frame.h>
#include <tui++/Panel.h>
#include <tui++/RootPane.h>
#include <tui++/Screen.h>
#include <tui++/TextField.h>

#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

using namespace tui;

namespace {

#define CHECK(condition) \
  do { \
    if (not(condition)) { \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
      std::abort(); \
    } \
  } while (0)

void drain() {
  while (screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
  }
}

std::shared_ptr<Event> dispatch_key(std::shared_ptr<Frame> const &frame, KeyEvent::Type type, KeyEvent::KeyCode code, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(frame, type, code, modifiers);
  auto event = screen.get_event_queue().pop();
  frame->dispatch_event(*event);
  drain();
  return event;
}

std::shared_ptr<Event> dispatch_typed(std::shared_ptr<Frame> const &frame, Char c, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(frame, c, modifiers);
  auto event = screen.get_event_queue().pop();
  frame->dispatch_event(*event);
  drain();
  return event;
}

// Presses and releases the left button at (x, y), window-local, through the
// window's normal dispatch path (the way the terminal input does).
void click_window(std::shared_ptr<Window> const &window, int x, int y, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, modifiers | InputEvent::LEFT_BUTTON_DOWN, x, y, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, modifiers, x, y, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  drain();
}

// The double-click that follows a press/release pair: the terminal synthesizes
// it with its click counter (see Terminal's mouse state machine).
void click_window_twice(std::shared_ptr<Window> const &window, int x, int y) {
  click_window(window, x, y);
  drain();
  screen.post<MouseClickEvent>(window, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, x, y, 2, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  drain();
}

}

// A field laid out inside a frame, focused, with its text replaced per
// scenario. Returns the frame; the field is reachable through it, but the
// tests keep their own handle.
static std::shared_ptr<Frame> show_field(std::shared_ptr<TextField> const &field) {
  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  field->set_bounds(1, 1, 40, 1);
  frame->get_content_pane()->add(field);
  frame->set_visible(true);
  drain();
  field->request_input_focus();
  return frame;
}

void test_text_field() {
  std::fprintf(stderr, "test_text_field: caret, selection, editing and clipboard commands\n");
  terminal.set_type("text");

  auto field = make_component<TextField>("hello world");
  auto frame = show_field(field);
  CHECK(field->is_focus_owner());

  // ---- content and caret ----
  CHECK(field->get_text() == "hello world");
  CHECK(field->get_caret_position() == 0); // Swing's setText leaves the caret at the start
  CHECK(not field->has_selection());

  field->set_caret_position(11);
  dispatch_typed(frame, Char('!'));
  CHECK(field->get_text() == "hello world!");
  CHECK(field->get_caret_position() == 12);

  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_SPACE);
  CHECK(field->get_text() == "hello world");

  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_HOME);
  CHECK(field->get_caret_position() == 0);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DELETE);
  CHECK(field->get_text() == "ello world");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_END);
  CHECK(field->get_caret_position() == 10);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_SPACE);
  CHECK(field->get_text() == "ello worl");

  // ---- Shift+arrows select; a plain arrow collapses the selection ----
  field->set_text("abcdef");
  field->set_caret_position(6);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT, InputEvent::SHIFT_DOWN);
  CHECK(field->get_caret_position() == 5);
  CHECK(field->get_selection_start() == 5 and field->get_selection_end() == 6);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT, InputEvent::SHIFT_DOWN);
  CHECK(field->get_selection_start() == 4 and field->get_selection_end() == 6);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT);
  CHECK(field->get_caret_position() == 4); // collapsed to the selection's edge
  CHECK(not field->has_selection());

  // Typing replaces the selection.
  field->set_text("abcdef");
  field->set_caret_position(6);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT, InputEvent::SHIFT_DOWN);
  CHECK(field->get_selection_start() == 5 and field->get_selection_end() == 6);
  dispatch_typed(frame, Char('X'));
  CHECK(field->get_text() == "abcdeX");
  CHECK(field->get_caret_position() == 6);

  // ---- select-all, then typing replaces everything ----
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_A, InputEvent::CTRL_DOWN);
  CHECK(field->get_selection_start() == 0 and field->get_selection_end() == 6);
  dispatch_typed(frame, Char('Z'));
  CHECK(field->get_text() == "Z");

  // Shift+Home / Shift+End select to the field's edges.
  field->set_text("abcdef");
  field->set_caret_position(6);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_HOME, InputEvent::SHIFT_DOWN);
  CHECK(field->has_selection() and field->get_selection_start() == 0 and field->get_selection_end() == 6);

  // ---- word-wise navigation and selection (Ctrl / Ctrl+Shift arrows) ----
  field->set_text("one two three");
  field->set_caret_position(field->get_text().size());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT, InputEvent::CTRL_DOWN);
  CHECK(field->get_caret_position() == 8); // the start of "three"
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT, InputEvent::CTRL_DOWN | InputEvent::SHIFT_DOWN);
  CHECK(field->get_selection_start() == 4 and field->get_selection_end() == 8); // "two" plus the space before it
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT, InputEvent::CTRL_DOWN);
  CHECK(field->get_caret_position() == 8);
  CHECK(not field->has_selection());

  // Ctrl+Backspace / Ctrl+Delete delete a word.
  field->set_text("one two");
  field->set_caret_position(7);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_SPACE, InputEvent::CTRL_DOWN);
  CHECK(field->get_text() == "one ");
  field->set_caret_position(0);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DELETE, InputEvent::CTRL_DOWN);
  CHECK(field->get_text() == " ");

  // ---- clipboard: copy, cut, paste (Ctrl and the classic chords) ----
  field->set_text("copy me");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_A, InputEvent::CTRL_DOWN);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_C, InputEvent::CTRL_DOWN);
  CHECK(Clipboard::get_text() == "copy me");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_X, InputEvent::CTRL_DOWN);
  CHECK(field->get_text().empty());
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_V, InputEvent::CTRL_DOWN);
  CHECK(field->get_text() == "copy me");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_V, InputEvent::CTRL_DOWN);
  CHECK(field->get_text() == "copy mecopy me");

  // Shift+Insert pastes and Ctrl+Insert copies (the Windows chords).
  field->set_text("chord");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_A, InputEvent::CTRL_DOWN);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_INSERT, InputEvent::CTRL_DOWN);
  CHECK(Clipboard::get_text() == "chord");
  field->set_text("");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_INSERT, InputEvent::SHIFT_DOWN);
  CHECK(field->get_text() == "chord");

  // Shift+Delete cuts (the classic Windows chord).
  field->set_text("cut me");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_A, InputEvent::CTRL_DOWN);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DELETE, InputEvent::SHIFT_DOWN);
  CHECK(field->get_text().empty());
  CHECK(Clipboard::get_text() == "cut me");

  // A multi-line paste into the single-line field flattens its line breaks.
  Clipboard::set_text("multi\r\nline\ntext");
  field->set_text("");
  field->paste();
  CHECK(field->get_text() == "multi line text");

  // A paste replaces the selection.
  Clipboard::set_text("new");
  field->set_text("old text");
  field->select(0, 3);
  field->paste();
  CHECK(field->get_text() == "new text");

  // ---- undo and redo (Ctrl+Z / Ctrl+Y) ----
  field->set_text("abc");
  field->set_caret_position(3);
  dispatch_typed(frame, Char('d'));
  CHECK(field->get_text() == "abcd");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_Z, InputEvent::CTRL_DOWN);
  CHECK(field->get_text() == "abc");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_Y, InputEvent::CTRL_DOWN);
  CHECK(field->get_text() == "abcd");

  // ---- Enter fires the ActionEvent (Swing's postActionEvent) ----
  auto actions = std::vector<std::string> { };
  auto listener = [&actions](ActionEvent &e) {
    actions.push_back(e.action_command);
  };
  field->add_listener(listener);
  field->set_text("commit me");
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(actions.size() == 1 and actions.back() == "commit me");

  // A field with a listener to notify keeps Enter; without one it leaves the
  // key to the window, where it clicks the window's default button (Swing's
  // NotifyAction is only enabled while there is a listener to accept the
  // field's content).
  auto ok = make_component<Button>("OK");
  auto ok_actions = 0;
  ok->add_listener([&ok_actions](ActionEvent &) {
    ++ok_actions;
  });
  frame->get_content_pane()->add(ok);
  frame->get_root_pane()->set_default_dutton(ok);
  drain();
  CHECK(field->is_focus_owner());

  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(actions.size() == 2);
  CHECK(ok_actions == 0);

  field->remove_listener(listener);
  // The stroke reaches the root pane's binding, which consumes it as it
  // clicks the button.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(ok_actions == 1);
  CHECK(actions.size() == 2);

  // ---- the mouse: click places the caret, drag selects, double-click
  // selects the word ----
  field->set_text("hello world");
  auto origin = convert_point_from_screen(field->get_location_on_screen(), frame);
  click_window(frame, origin.x + 1 + 2, origin.y); // the cell of the third glyph
  CHECK(field->get_caret_position() == 2);
  CHECK(not field->has_selection());

  // The drags arrive through the screen listener (the press registered it),
  // so the press is dispatched without its release: the button stays down
  // while the drags extend the selection from the press point.
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, origin.x + 1 + 2, origin.y, false);
  frame->dispatch_event(*screen.get_event_queue().pop());
  auto drag = std::make_shared<MouseDragEvent>(field, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, 1 + 5, 0);
  screen.notify_listeners(*drag);
  CHECK(field->get_selection_start() == 2 and field->get_selection_end() == 5);
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, origin.x + 1 + 5, origin.y, false);
  frame->dispatch_event(*screen.get_event_queue().pop());
  drain();

  // A double-click selects the word under the pointer.
  click_window_twice(frame, origin.x + 1 + 8, origin.y); // inside "world"
  CHECK(field->get_selection_start() == 6 and field->get_selection_end() == 11);
  CHECK(field->get_text() == "hello world"); // selecting changed nothing

  // ---- read-only mode: the caret and the selection still work, the edits
  // do not ----
  field->set_editable(false);
  field->set_text("read only");
  dispatch_typed(frame, Char('!'));
  CHECK(field->get_text() == "read only");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_SPACE);
  CHECK(field->get_text() == "read only");
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_HOME);
  CHECK(field->get_caret_position() == 0);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_END, InputEvent::SHIFT_DOWN);
  CHECK(field->has_selection());
  field->copy();
  CHECK(Clipboard::get_text() == "read only");
  field->set_editable(true);

  // ---- columns size the preferred width ----
  field->set_text("");
  field->set_columns(10);
  auto narrow = field->get_preferred_size();
  field->set_columns(20);
  auto wide = field->get_preferred_size();
  CHECK(wide.width > narrow.width);

  frame->set_visible(false);
  drain();
  std::fprintf(stderr, "test_text_field: ok\n");
}
