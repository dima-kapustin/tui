// Tests of the terminal input translation: the pure decoders that turn SGR
// mouse reports and CSI key sequences into event parameters, and the mouse
// state machine the decoders feed.
//
// The translations under test are the ones the InputParser (Terminal.cpp /
// Terminal-InputParser.cpp) performs; the parser itself reads its bytes from
// the platform input reader and is not reachable from unit tests, so the
// decode steps and the new_*_event entry points it drives are public and are
// exercised here with the report/sequence parameters the parser would pass.
//
// The regression this suite was written for: motion reports (SGR codes
// 32..35, mode 1003) carry the X11 button numbering of the button HELD, and
// code 3 -- the X10 release marker -- means NO button (a plain move). A
// decoder that read code 3 as a held right button turned every buttonless
// mouse move into a right-button drag; the sticky button modifier then made
// presses look like drag continuations, the window dispatcher never
// retargeted them to the component under the pointer, and menu popups could
// not be opened by clicking. The cases below pin the mapping (35 = move,
// 32/33/34 = left/middle/right drag) and the state transitions that keep a
// press after moves/drags free of phantom button modifiers.

#include <tui++/Screen.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

using namespace tui;

#define CHECK(cond)                                                                                                    \
  do {                                                                                                                 \
    if (not(cond)) {                                                                                                   \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
      std::abort();                                                                                                    \
    }                                                                                                                  \
  } while (0)

// Pops and discards every queued event, so pending invocations from earlier
// tests (repaints, timers) are never mistaken for translation output.
static void drain() {
  while (screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
  }
}

// Pops one queued event; fails when the queue is empty or the event is not
// of the expected type.
template<typename T>
static std::shared_ptr<T> pop_as(unsigned expected_id) {
  auto event = screen.get_event_queue().pop(std::chrono::milliseconds::zero());
  CHECK(event != nullptr);
  CHECK(event->id == expected_id);
  auto typed = std::dynamic_pointer_cast<T>(event);
  CHECK(typed != nullptr);
  return typed;
}

static void expect_no_pending_events() {
  auto event = screen.get_event_queue().pop(std::chrono::milliseconds::zero());
  CHECK(event == nullptr);
}

static void expect_modifiers(InputEvent::Modifiers actual, InputEvent::Modifiers expected) {
  CHECK(actual == expected);
}

// ---------------------------------------------------------------------------
// SGR mouse report decode: code -> kind/button/wheel/modifiers.

struct MouseRow {
  unsigned code;
  bool pressed;
  Terminal::MouseReport::Kind kind;
  MousePressEvent::Button button;
  int wheel_rotation;
  InputEvent::Modifiers modifiers;
};

static void test_mouse_report_decode() {
  // Presses and releases echo the X11 button numbering (0 = left, 1 = middle,
  // 2 = right); the terminator (M/m) selects press over release. Code 3 is
  // the X10 release marker -- not a button a press/release carries.
  const MouseRow press_release[] = {
    { 0, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::LEFT_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 1, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::MIDDLE_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 2, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::RIGHT_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 3, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::NO_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 0, false, Terminal::MouseReport::Kind::RELEASE, MousePressEvent::LEFT_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 1, false, Terminal::MouseReport::Kind::RELEASE, MousePressEvent::MIDDLE_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 2, false, Terminal::MouseReport::Kind::RELEASE, MousePressEvent::RIGHT_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 3, false, Terminal::MouseReport::Kind::RELEASE, MousePressEvent::NO_BUTTON, 0, InputEvent::NO_MODIFIERS },
    // Shift/Alt(Meta)/Ctrl ride in bits 2..4: Alt+left press (the column
    // selection modifier) is 8 + 0.
    { 4, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::LEFT_BUTTON, 0, InputEvent::SHIFT_DOWN },
    { 8, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::LEFT_BUTTON, 0, InputEvent::META_DOWN },
    { 16, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::LEFT_BUTTON, 0, InputEvent::CTRL_DOWN },
    { 28, true, Terminal::MouseReport::Kind::PRESS, MousePressEvent::LEFT_BUTTON, 0, InputEvent::SHIFT_DOWN | InputEvent::META_DOWN | InputEvent::CTRL_DOWN },
    { 12, false, Terminal::MouseReport::Kind::RELEASE, MousePressEvent::LEFT_BUTTON, 0, InputEvent::SHIFT_DOWN | InputEvent::META_DOWN },
  };
  for (auto &&row : press_release) {
    auto report = Terminal::decode_mouse_report(row.code, row.pressed);
    CHECK(report.kind == row.kind);
    CHECK(report.button == row.button);
    CHECK(report.wheel_rotation == row.wheel_rotation);
    expect_modifiers(report.key_modifiers, row.modifiers);
  }

  // Motion reports (bit 5, mode 1003): the low two bits identify the button
  // HELD during the motion; code 3 means no button is held -- a plain move,
  // which the hover tracking needs. (An earlier version read code 3 as a
  // right-button drag; the Windows console and Windows Terminal send 35 for
  // every buttonless move, so all hover/clicks broke.)
  const MouseRow motion[] = {
    { 32, true, Terminal::MouseReport::Kind::DRAG, MousePressEvent::LEFT_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 33, true, Terminal::MouseReport::Kind::DRAG, MousePressEvent::MIDDLE_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 34, true, Terminal::MouseReport::Kind::DRAG, MousePressEvent::RIGHT_BUTTON, 0, InputEvent::NO_MODIFIERS },
    { 35, true, Terminal::MouseReport::Kind::MOVE, MousePressEvent::NO_BUTTON, 0, InputEvent::NO_MODIFIERS },
    // The terminator is ignored for motion reports (they only arrive as M).
    { 35, false, Terminal::MouseReport::Kind::MOVE, MousePressEvent::NO_BUTTON, 0, InputEvent::NO_MODIFIERS },
    // Modified drags: Alt+left drag (column selection) is 32 + 8 + 0 = 40.
    { 36, true, Terminal::MouseReport::Kind::DRAG, MousePressEvent::LEFT_BUTTON, 0, InputEvent::SHIFT_DOWN },
    { 40, true, Terminal::MouseReport::Kind::DRAG, MousePressEvent::LEFT_BUTTON, 0, InputEvent::META_DOWN },
    { 48, true, Terminal::MouseReport::Kind::DRAG, MousePressEvent::LEFT_BUTTON, 0, InputEvent::CTRL_DOWN },
    { 63, true, Terminal::MouseReport::Kind::MOVE, MousePressEvent::NO_BUTTON, 0, InputEvent::SHIFT_DOWN | InputEvent::META_DOWN | InputEvent::CTRL_DOWN },
  };
  for (auto &&row : motion) {
    auto report = Terminal::decode_mouse_report(row.code, row.pressed);
    CHECK(report.kind == row.kind);
    CHECK(report.button == row.button);
    expect_modifiers(report.key_modifiers, row.modifiers);
  }

  // Wheel reports (bit 6): button 0 = up (-1), anything else = down (+1),
  // with the same modifier bits (Shift+wheel scrolls horizontally).
  const MouseRow wheel[] = {
    { 64, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, -1, InputEvent::NO_MODIFIERS },
    { 65, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, 1, InputEvent::NO_MODIFIERS },
    { 66, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, 1, InputEvent::NO_MODIFIERS },
    { 67, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, 1, InputEvent::NO_MODIFIERS },
    { 68, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, -1, InputEvent::SHIFT_DOWN },
    { 72, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, -1, InputEvent::META_DOWN },
    { 80, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, -1, InputEvent::CTRL_DOWN },
    { 95, true, Terminal::MouseReport::Kind::WHEEL, MousePressEvent::NO_BUTTON, 1, InputEvent::SHIFT_DOWN | InputEvent::META_DOWN | InputEvent::CTRL_DOWN },
  };
  for (auto &&row : wheel) {
    auto report = Terminal::decode_mouse_report(row.code, row.pressed);
    CHECK(report.kind == row.kind);
    CHECK(report.wheel_rotation == row.wheel_rotation);
    expect_modifiers(report.key_modifiers, row.modifiers);
  }
}

// ---------------------------------------------------------------------------
// CSI key decode: the modifier parameter of ESC [ 1 ; <param> X and the
// selectors that carry keys.

static void test_csi_key_decode() {
  // xterm encodes the modifiers as 1 + a bit mask (1 = Shift, 2 = Alt,
  // 4 = Ctrl, 8 = Meta); Ctrl+Up arrives as ESC [ 1 ; 5 A.
  auto expect_key_mods = [](std::vector<unsigned> const &params, InputEvent::Modifiers expected) {
    expect_modifiers(Terminal::decode_csi_key_modifiers(params), expected);
  };
  expect_key_mods({ }, InputEvent::NO_MODIFIERS);
  expect_key_mods({ 1 }, InputEvent::NO_MODIFIERS);
  expect_key_mods({ 1, 1 }, InputEvent::NO_MODIFIERS);
  expect_key_mods({ 1, 0 }, InputEvent::NO_MODIFIERS);
  expect_key_mods({ 1, 2 }, InputEvent::SHIFT_DOWN);
  expect_key_mods({ 1, 3 }, InputEvent::ALT_DOWN);
  expect_key_mods({ 1, 4 }, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN);
  expect_key_mods({ 1, 5 }, InputEvent::CTRL_DOWN); // Ctrl+arrow / Ctrl+Home/End
  expect_key_mods({ 1, 6 }, InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN);
  expect_key_mods({ 1, 7 }, InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN);
  expect_key_mods({ 1, 8 }, InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN);
  expect_key_mods({ 1, 9 }, InputEvent::META_DOWN);
  expect_key_mods({ 1, 10 }, InputEvent::SHIFT_DOWN | InputEvent::META_DOWN);
  expect_key_mods({ 1, 11 }, InputEvent::ALT_DOWN | InputEvent::META_DOWN);
  expect_key_mods({ 1, 12 }, InputEvent::SHIFT_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN);
  expect_key_mods({ 1, 13 }, InputEvent::CTRL_DOWN | InputEvent::META_DOWN);
  expect_key_mods({ 1, 14 }, InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::META_DOWN);
  expect_key_mods({ 1, 15 }, InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN);
  expect_key_mods({ 1, 16 }, InputEvent::SHIFT_DOWN | InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN);
  expect_key_mods({ 1, 17 }, InputEvent::NO_MODIFIERS);

  auto expect_key = [](std::vector<unsigned> const &params, char selector, KeyEvent::KeyCode expected) {
    auto key = Terminal::decode_csi_key(params, selector);
    CHECK(key.has_value());
    CHECK(key.value() == expected);
  };
  // Cursor keys, and Home/End -- including the modified forms (ESC [ 1 ; 5 H
  // = Ctrl+Home), which used to be dropped.
  expect_key({ }, 'A', KeyEvent::VK_UP);
  expect_key({ }, 'B', KeyEvent::VK_DOWN);
  expect_key({ }, 'C', KeyEvent::VK_RIGHT);
  expect_key({ }, 'D', KeyEvent::VK_LEFT);
  expect_key({ }, 'H', KeyEvent::VK_HOME);
  expect_key({ }, 'F', KeyEvent::VK_END);
  expect_key({ 1 }, 'A', KeyEvent::VK_UP);
  expect_key({ 1, 5 }, 'A', KeyEvent::VK_UP);
  expect_key({ 1, 5 }, 'C', KeyEvent::VK_RIGHT);
  expect_key({ 1, 5 }, 'H', KeyEvent::VK_HOME);
  expect_key({ 1, 5 }, 'F', KeyEvent::VK_END);
  expect_key({ 1, 2 }, 'D', KeyEvent::VK_LEFT);

  // The numbered keys, ESC [ <code> ~.
  expect_key({ 2 }, '~', KeyEvent::VK_INSERT);
  expect_key({ 3 }, '~', KeyEvent::VK_DELETE);
  expect_key({ 5 }, '~', KeyEvent::VK_PAGE_UP);
  expect_key({ 6 }, '~', KeyEvent::VK_PAGE_DOWN);
  expect_key({ 15 }, '~', KeyEvent::VK_F5);
  expect_key({ 17 }, '~', KeyEvent::VK_F6);
  expect_key({ 18 }, '~', KeyEvent::VK_F7);
  expect_key({ 19 }, '~', KeyEvent::VK_F8);
  expect_key({ 20 }, '~', KeyEvent::VK_F9);
  expect_key({ 21 }, '~', KeyEvent::VK_F10);
  expect_key({ 23 }, '~', KeyEvent::VK_F11);
  expect_key({ 24 }, '~', KeyEvent::VK_F12);
  expect_key({ 5, 5 }, '~', KeyEvent::VK_PAGE_UP); // Ctrl+PageUp
  expect_key({ 5, 2 }, '~', KeyEvent::VK_PAGE_UP); // Shift+PageUp

  // Selectors that are not keys (mouse reports, cursor position reports,
  // unknown finals) decode to nothing, as do the unmapped legacy tilde keys.
  auto expect_no_key = [](std::vector<unsigned> const &params, char selector) {
    CHECK(not Terminal::decode_csi_key(params, selector).has_value());
  };
  expect_no_key({ 0, 3, 1 }, 'M');
  expect_no_key({ 0, 3, 1 }, 'm');
  expect_no_key({ }, 'R');
  expect_no_key({ }, 'Z');
  expect_no_key({ }, 'X');
  expect_no_key({ }, '~');
  expect_no_key({ 0 }, '~');
  expect_no_key({ 1 }, '~'); // Find
  expect_no_key({ 4 }, '~'); // Select
  expect_no_key({ 7 }, '~');
  expect_no_key({ 28 }, '~'); // Help
  expect_no_key({ 29 }, '~'); // Menu
}

// ---------------------------------------------------------------------------
// The mouse state machine behind the decoders: button-modifier tracking,
// duplicate suppression, click synthesis and the legacy motion inference.
//
// The scenarios below share one Terminal (and therefore its input state), so
// they run as one sequence and every scenario starts from the state the
// previous one ended in -- the expected event streams are written for that
// exact order.

static void test_mouse_state_machine() {
  terminal.set_type("text");
  drain();

  // Scenario 1 -- the reported regression: a plain buttonless move must stay
  // a move (code 35), and a click after moves must reach the component under
  // the pointer: its press carries only its own button, no phantom modifiers,
  // so was_button_down_before() is false and the window dispatcher retargets
  // it to the menu/component under the cursor.
  terminal.new_mouse_move_event(InputEvent::NO_MODIFIERS, 1, 1);
  terminal.new_mouse_move_event(InputEvent::NO_MODIFIERS, 2, 2);
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 3, 3);
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 3, 3);

  {
    auto move1 = pop_as<MouseMoveEvent>(MouseMoveEvent::MOUSE_MOVED);
    CHECK(move1->x == 1 and move1->y == 1);
    expect_modifiers(move1->modifiers, InputEvent::NO_MODIFIERS);
    auto move2 = pop_as<MouseMoveEvent>(MouseMoveEvent::MOUSE_MOVED);
    CHECK(move2->x == 2 and move2->y == 2);

    auto press = pop_as<MousePressEvent>(MousePressEvent::MOUSE_PRESSED);
    CHECK(press->button == MousePressEvent::LEFT_BUTTON);
    CHECK(press->x == 3 and press->y == 3);
    // The whole reported bug: the press must not carry a right (or middle)
    // button, and must not look like a drag continuation.
    expect_modifiers(press->modifiers, InputEvent::LEFT_BUTTON_DOWN);
    CHECK(not press->was_button_down_before());

    auto release = pop_as<MousePressEvent>(MousePressEvent::MOUSE_RELEASED);
    CHECK(release->button == MousePressEvent::LEFT_BUTTON);
    // The release clears its own button modifier again.
    expect_modifiers(release->modifiers, InputEvent::NO_MODIFIERS);

    auto click = pop_as<MouseClickEvent>(MouseClickEvent::MOUSE_CLICKED);
    CHECK(click->button == MousePressEvent::LEFT_BUTTON);
  }
  expect_no_pending_events();

  // Scenario 2 -- a real drag sequence (press, held-button motion, release)
  // leaves the state clean: the release clears the button, and the next
  // press is a fresh press, not a drag continuation.
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 5, 5);
  terminal.new_mouse_drag_event(MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 6, 5);
  terminal.new_mouse_drag_event(MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 7, 6);
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 7, 6);
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 8, 8);
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 8, 8);

  {
    auto press = pop_as<MousePressEvent>(MousePressEvent::MOUSE_PRESSED);
    CHECK(press->x == 5 and press->y == 5);
    auto drag1 = pop_as<MouseDragEvent>(MouseDragEvent::MOUSE_DRAGGED);
    CHECK(drag1->button == MousePressEvent::LEFT_BUTTON);
    CHECK(drag1->x == 6 and drag1->y == 5);
    expect_modifiers(drag1->modifiers, InputEvent::LEFT_BUTTON_DOWN);
    auto drag2 = pop_as<MouseDragEvent>(MouseDragEvent::MOUSE_DRAGGED);
    CHECK(drag2->x == 7 and drag2->y == 6);
    auto release = pop_as<MousePressEvent>(MousePressEvent::MOUSE_RELEASED);
    expect_modifiers(release->modifiers, InputEvent::NO_MODIFIERS);
    pop_as<MouseClickEvent>(MouseClickEvent::MOUSE_CLICKED);

    // The fresh press after the drag: clean modifiers, not a drag
    // continuation (was_button_down_before() XORs only its own button).
    auto press2 = pop_as<MousePressEvent>(MousePressEvent::MOUSE_PRESSED);
    CHECK(press2->x == 8 and press2->y == 8);
    expect_modifiers(press2->modifiers, InputEvent::LEFT_BUTTON_DOWN);
    CHECK(not press2->was_button_down_before());
    pop_as<MousePressEvent>(MousePressEvent::MOUSE_RELEASED);
    pop_as<MouseClickEvent>(MouseClickEvent::MOUSE_CLICKED);
  }
  expect_no_pending_events();

  // Scenario 3 -- a release the terminal never reported (the pointer left
  // the terminal window mid-drag) leaves the button modifier stuck; the next
  // buttonless move clears it again (a move report means no button is held),
  // so a later press cannot be mistaken for a drag continuation.
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 10, 10);
  terminal.new_mouse_drag_event(MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 11, 10);
  terminal.new_mouse_move_event(InputEvent::NO_MODIFIERS, 12, 12);
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 10, 10);

  {
    pop_as<MousePressEvent>(MousePressEvent::MOUSE_PRESSED);
    auto drag = pop_as<MouseDragEvent>(MouseDragEvent::MOUSE_DRAGGED);
    expect_modifiers(drag->modifiers, InputEvent::LEFT_BUTTON_DOWN);
    // The self-healing move: the stuck LEFT_BUTTON_DOWN is gone.
    auto move = pop_as<MouseMoveEvent>(MouseMoveEvent::MOUSE_MOVED);
    CHECK(move->x == 12 and move->y == 12);
    expect_modifiers(move->modifiers, InputEvent::NO_MODIFIERS);
    pop_as<MousePressEvent>(MousePressEvent::MOUSE_RELEASED);
    pop_as<MouseClickEvent>(MouseClickEvent::MOUSE_CLICKED);
  }
  expect_no_pending_events();

  // Scenario 4 -- duplicate suppression: an identical repeat of the last
  // press (same type, button and cell; some terminals double-report) is
  // dropped, while a press of the same button at a NEW cell is a drag: the
  // legacy motion inference for terminals that do not report motion
  // (they re-press at every cell the drag crosses instead).
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 10, 10);
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 10, 10); // duplicate: dropped
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 12, 12); // new cell: drag
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 12, 12);

  {
    // Exactly one press: the duplicate was dropped, and the press at the new
    // cell became a drag (the legacy motion inference).
    auto press = pop_as<MousePressEvent>(MousePressEvent::MOUSE_PRESSED);
    CHECK(press->x == 10 and press->y == 10);
    auto drag = pop_as<MouseDragEvent>(MouseDragEvent::MOUSE_DRAGGED);
    CHECK(drag->button == MousePressEvent::LEFT_BUTTON);
    CHECK(drag->x == 12 and drag->y == 12);
    auto release = pop_as<MousePressEvent>(MousePressEvent::MOUSE_RELEASED);
    CHECK(release->x == 12 and release->y == 12);
    expect_modifiers(release->modifiers, InputEvent::NO_MODIFIERS);
    pop_as<MouseClickEvent>(MouseClickEvent::MOUSE_CLICKED);
  }
  expect_no_pending_events();

  // Scenario 5 -- a release at a new cell (nothing was pressed there) is the
  // terminal's way of reporting motion after the button went up: it decodes
  // as a move.
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 14, 14);
  {
    auto move = pop_as<MouseMoveEvent>(MouseMoveEvent::MOUSE_MOVED);
    CHECK(move->x == 14 and move->y == 14);
    expect_modifiers(move->modifiers, InputEvent::NO_MODIFIERS);
  }
  expect_no_pending_events();

  // Scenario 6 -- click synthesis: a release shortly after its press fires a
  // MouseClickEvent; a second press/release pair inside the double-click
  // window reports click_count 2. (The sleep clears the double-click window
  // so the first click of the pair is a single click no matter how fast the
  // earlier scenarios ran.)
  std::this_thread::sleep_for(std::chrono::milliseconds(350));
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 5, 5);
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 5, 5);
  terminal.new_mouse_event(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 5, 5);
  terminal.new_mouse_event(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 5, 5);
  {
    pop_as<MousePressEvent>(MousePressEvent::MOUSE_PRESSED);
    pop_as<MousePressEvent>(MousePressEvent::MOUSE_RELEASED);
    auto click1 = pop_as<MouseClickEvent>(MouseClickEvent::MOUSE_CLICKED);
    CHECK(click1->click_count == 1);
    pop_as<MousePressEvent>(MousePressEvent::MOUSE_PRESSED);
    pop_as<MousePressEvent>(MousePressEvent::MOUSE_RELEASED);
    auto click2 = pop_as<MouseClickEvent>(MouseClickEvent::MOUSE_CLICKED);
    CHECK(click2->click_count == 2);
  }
  expect_no_pending_events();

  // Scenario 7 -- wheel events pass their rotation and modifiers through
  // without touching the button state.
  terminal.new_mouse_wheel_event(1, InputEvent::SHIFT_DOWN, 5, 5);
  terminal.new_mouse_wheel_event(-1, InputEvent::NO_MODIFIERS, 6, 6);
  {
    auto wheel1 = pop_as<MouseWheelEvent>(MouseWheelEvent::MOUSE_WHEEL);
    CHECK(wheel1->wheel_rotation == 1);
    CHECK(wheel1->x == 5 and wheel1->y == 5);
    expect_modifiers(wheel1->modifiers, InputEvent::SHIFT_DOWN);
    auto wheel2 = pop_as<MouseWheelEvent>(MouseWheelEvent::MOUSE_WHEEL);
    CHECK(wheel2->wheel_rotation == -1);
    expect_modifiers(wheel2->modifiers, InputEvent::NO_MODIFIERS);
  }
  expect_no_pending_events();
}

static void test_key_posting() {
  // The keyboard entry points post the decoded key events: action keys carry
  // their key code and modifiers (Ctrl+Right is how the text area's
  // word-jump arrives), typed characters arrive as KEY_TYPED events.
  terminal.new_key_event(KeyEvent::VK_RIGHT, InputEvent::CTRL_DOWN);
  terminal.new_key_event(KeyEvent::VK_UP, InputEvent::NO_MODIFIERS);
  terminal.new_key_event(Char { 'x' }, InputEvent::NO_MODIFIERS);
  terminal.new_key_event(KeyEvent::VK_F8, InputEvent::NO_MODIFIERS);

  auto key1 = pop_as<KeyEvent>(KeyEvent::KEY_PRESSED);
  CHECK(key1->get_key_code() == KeyEvent::VK_RIGHT);
  expect_modifiers(key1->modifiers, InputEvent::CTRL_DOWN);

  auto key2 = pop_as<KeyEvent>(KeyEvent::KEY_PRESSED);
  CHECK(key2->get_key_code() == KeyEvent::VK_UP);
  expect_modifiers(key2->modifiers, InputEvent::NO_MODIFIERS);

  auto typed = pop_as<KeyEvent>(KeyEvent::KEY_TYPED);
  CHECK(typed->get_key_char() == Char { 'x' });

  auto key3 = pop_as<KeyEvent>(KeyEvent::KEY_PRESSED);
  CHECK(key3->get_key_code() == KeyEvent::VK_F8);
  expect_no_pending_events();
}

void test_InputTranslation() {
  test_mouse_report_decode();
  test_csi_key_decode();
  test_mouse_state_machine();
  test_key_posting();
  std::fprintf(stderr, "test_InputTranslation: ok\n");
}
