// Exercises the component context menu -- Swing's componentPopupMenu and the
// popup trigger that opens it -- and the standard text popup the toolkit
// gives every text component:
//
//   * a right-button press on a component shows the menu it carries (or the
//     nearest ancestor's when it inherits one) at the mouse,
//   * the menu a text component carries for free is the Swing editing set
//     (Undo, Redo, Cut, Copy, Paste, Delete, Select All) and the rows enable
//     themselves from the component's state when the menu shows,
//   * the popup is modal for the keyboard while it is up: the arrows walk its
//     rows, Enter activates, Escape closes, letters select mnemonics and the
//     editing keys do not reach the text behind it,
//   * a press outside the popup dismisses it, a press inside it picks a row.
//
// The whole project (and therefore this test) is usually built with NDEBUG
// (Release), which would compile the assert()s out and make the test pass
// vacuously; CHECK below aborts regardless, so the test always verifies.
#include <tui++/Clipboard.h>
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/PopupMenu.h>
#include <tui++/Screen.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/TextField.h>

#include <tui++/event/Event.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

using namespace tui;

#define CHECK(cond)                                                                                                    \
  do {                                                                                                                 \
    if (not(cond)) {                                                                                                   \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
      std::abort();                                                                                                    \
    }                                                                                                                  \
  } while (0)

static void drain() {
  while (screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
  }
}

// Presses the right button at (x, y) window-local -- the terminal's popup
// trigger (see Terminal: the right press is the trigger, the release is not).
static void right_press(std::shared_ptr<Window> const &window, int x, int y) {
  drain();
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_PRESSED, MousePressEvent::RIGHT_BUTTON, InputEvent::RIGHT_BUTTON_DOWN, x, y, true);
  window->dispatch_event(*screen.get_event_queue().pop());
  drain();
}

// Presses the left button at (x, y) window-local (no popup trigger).
static void left_press(std::shared_ptr<Window> const &window, int x, int y) {
  drain();
  screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, x, y, false);
  window->dispatch_event(*screen.get_event_queue().pop());
  drain();
}

static std::shared_ptr<Event> dispatch_key(std::shared_ptr<Window> const &window, KeyEvent::Type type, KeyEvent::KeyCode code, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(window, type, code, modifiers);
  auto event = screen.get_event_queue().pop();
  window->dispatch_event(*event);
  drain();
  return event;
}

// The row of `popup` whose label is `text`.
static std::shared_ptr<MenuItem> row(std::shared_ptr<PopupMenu> const &popup, std::string const &text) {
  for (auto &&component : popup->get_components()) {
    if (auto item = std::dynamic_pointer_cast<MenuItem>(component); item and item->get_text() == text) {
      return item;
    }
  }
  return {};
}

static bool armed(std::shared_ptr<PopupMenu> const &popup, std::string const &text) {
  auto item = row(popup, text);
  return item and item->is_armed();
}

// The standard popup of a text component: the Swing editing set, in the Swing
// order, with the separators between the groups.
static void test_standard_text_popup() {
  terminal.set_type("text");
  drain();

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto field = make_component<TextField>("hello world", 20);
  frame->add(field);
  frame->set_visible(true);
  drain();

  // The Clipboard is process-global: a previous test may have left text in it,
  // and the popup's Paste row follows it.
  Clipboard::set_text("");

  // The field carries the standard popup from birth (BasicTextUI installs one
  // the same way): nothing has to be wired for a program to get it.
  auto popup = field->get_component_popup_menu();
  CHECK(popup != nullptr);
  CHECK(not popup->is_popup_showing());
  CHECK(not popup->is_context_menu());

  auto rows = std::vector<std::string> { };
  for (auto &&component : popup->get_components()) {
    if (auto item = std::dynamic_pointer_cast<MenuItem>(component)) {
      rows.push_back(item->get_text());
    } else {
      rows.push_back("-");
    }
  }
  CHECK((rows == std::vector<std::string> { "Undo", "Redo", "-", "Cut", "Copy", "Paste", "Delete", "-", "Select All" }));

  // The popup trigger opens it right over the mouse, as a context menu (the
  // menu system may drive this one; a widget's own dropdown it leaves alone).
  auto field_at = field->get_location_on_screen();
  auto frame_at = frame->get_location_on_screen();
  auto local = Point { field_at.x - frame_at.x + 2, field_at.y - frame_at.y };
  right_press(frame, local.x, local.y);
  CHECK(popup->is_popup_showing());
  CHECK(popup->is_context_menu());
  CHECK(popup->get_invoker() == field);

  // The rows enable themselves from the component's state when the menu
  // shows: with nothing selected and nothing to paste, only the two
  // selection-independent rows are live.
  CHECK(row(popup, "Undo") and not row(popup, "Undo")->is_enabled());
  CHECK(row(popup, "Redo") and not row(popup, "Redo")->is_enabled());
  CHECK(row(popup, "Cut") and not row(popup, "Cut")->is_enabled());
  CHECK(row(popup, "Copy") and not row(popup, "Copy")->is_enabled());
  CHECK(row(popup, "Paste") and not row(popup, "Paste")->is_enabled());
  CHECK(row(popup, "Select All") and row(popup, "Select All")->is_enabled());

  // Escape closes the popup and the key is consumed by the menu.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);
  CHECK(not popup->is_popup_showing());
  CHECK(not popup->is_context_menu());

  // A press on the component again re-opens it (and the press that opens a
  // popup is not delivered to the component as a click).
  right_press(frame, local.x, local.y);
  CHECK(popup->is_popup_showing());

  // The arrows walk the enabled rows and Enter activates the armed one. With
  // an empty history and no selection the live rows are Delete (the field is
  // editable, so the row is live) and Select All: the first Down arms Delete,
  // the second Select All, and Enter runs it.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(armed(popup, "Delete"));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(armed(popup, "Select All"));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(not popup->is_popup_showing());
  CHECK(field->has_selection());
  CHECK(field->get_selection_start() == 0 and field->get_selection_end() == field->get_text().size());

  // With a selection the clipboard rows are live the next time the menu
  // shows, and a pick runs the command on the field.
  right_press(frame, local.x, local.y);
  CHECK(row(popup, "Cut")->is_enabled());
  CHECK(row(popup, "Copy")->is_enabled());
  CHECK(not row(popup, "Paste")->is_enabled());
  row(popup, "Copy")->do_click(std::chrono::milliseconds::zero());
  CHECK(Clipboard::get_text() == "hello world");

  // The editing keys do not reach the text behind an open popup (the popup is
  // modal for the keyboard), and a letter no row claims is swallowed.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DELETE)->consumed);
  CHECK(field->get_text() == "hello world");
  CHECK(popup->is_popup_showing());

  // A press outside the popup dismisses it; a press inside it picks the row
  // under the pointer (a row pick runs through the menu machinery).
  left_press(frame, 0, frame->get_height() - 1);
  CHECK(not popup->is_popup_showing());

  // A read-only field keeps the popup but disables every row that would edit.
  field->set_editable(false);
  Clipboard::set_text("elsewhere");
  right_press(frame, local.x, local.y);
  CHECK(not row(popup, "Cut")->is_enabled());
  CHECK(not row(popup, "Paste")->is_enabled());
  CHECK(not row(popup, "Delete")->is_enabled());
  CHECK(row(popup, "Copy")->is_enabled());
  CHECK(row(popup, "Select All")->is_enabled());
  popup->set_visible(false);
  field->set_editable(true);

  frame->set_visible(false);
  drain();
}

// The text area carries the same standard popup, and a read-only one keeps its
// selection rows live: viewers (a log pane, a diff view) get Copy and Select
// All without wiring anything.
static void test_standard_text_area_popup() {
  terminal.set_type("text");
  drain();

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto area = make_component<TextArea>();
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, "alpha\nbeta\ngamma\n");
  buffer->scan_to_end();
  area->set_buffer(buffer);
  area->set_readonly(true);
  frame->add(area);
  frame->set_visible(true);
  drain();

  auto popup = area->get_component_popup_menu();
  CHECK(popup != nullptr);
  CHECK(popup != nullptr and popup->is_context_menu() == false);

  Clipboard::set_text("");
  auto at = area->get_location_on_screen();
  auto frame_at = frame->get_location_on_screen();
  right_press(frame, at.x - frame_at.x + 1, at.y - frame_at.y + 1);
  CHECK(popup->is_popup_showing());
  CHECK(popup->get_invoker() == area);

  // Read-only: every editing row is out, the selection rows are in.
  CHECK(not row(popup, "Undo")->is_enabled());
  CHECK(not row(popup, "Redo")->is_enabled());
  CHECK(not row(popup, "Cut")->is_enabled());
  CHECK(not row(popup, "Paste")->is_enabled());
  CHECK(not row(popup, "Delete")->is_enabled());
  CHECK(not row(popup, "Copy")->is_enabled()); // nothing selected yet
  CHECK(row(popup, "Select All")->is_enabled());

  // Select All through the menu, then copy through it.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(armed(popup, "Select All"));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(area->has_selection());

  right_press(frame, at.x - frame_at.x + 1, at.y - frame_at.y + 1);
  CHECK(row(popup, "Copy")->is_enabled());
  row(popup, "Copy")->do_click(std::chrono::milliseconds::zero());
  CHECK(Clipboard::get_text() == "alpha\nbeta\ngamma\n");
  popup->set_visible(false);

  frame->set_visible(false);
  drain();
}

// Shift+F10 is the popup trigger on the keyboard (Swing's KeyEvent.VK_F10 is
// its popup trigger key): it opens the focus owner's menu wherever the caret
// is. A program can also replace the menu a text component carries with one
// of its own (JComponent.setComponentPopupMenu); the standard menu serves
// until it does.
static void test_popup_trigger_key_and_custom_menu() {
  terminal.set_type("text");
  drain();

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto field = make_component<TextField>("abc", 10);
  frame->add(field);
  frame->set_visible(true);
  drain();

  field->request_input_focus();
  drain();

  // Shift+F10 opens the standard text menu at the caret, over the field.
  auto popup = field->get_component_popup_menu();
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10, InputEvent::SHIFT_DOWN)->consumed);
  CHECK(popup->is_popup_showing());

  // A program menu replaces the standard one (JComponent.setComponentPopupMenu).
  auto custom = make_component<PopupMenu>();
  auto custom_item = custom->add("Zap");
  auto zaps = 0;
  custom_item->add_listener([&zaps](ActionEvent &) {
    ++zaps;
  });
  field->set_component_popup_menu(custom);
  CHECK(field->get_component_popup_menu() == custom);
  // The standard menu that is already on the screen stays until it is
  // dismissed (Swing's setComponentPopupMenu does not touch a menu that is
  // showing) and the next trigger opens the program's menu instead.
  CHECK(popup->is_popup_showing());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);
  CHECK(not popup->is_popup_showing());

  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10, InputEvent::SHIFT_DOWN)->consumed);
  CHECK(custom->is_popup_showing());
  CHECK(not popup->is_popup_showing());

  // The custom menu's rows are ordinary rows: Enter on the armed one runs it.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(armed(custom, "Zap"));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(zaps == 1);

  frame->set_visible(false);
  drain();
}

// A component without a menu of its own offers its ancestors' when it is told
// to inherit them (Swing's setInheritsPopupMenu); by default it does not.
static void test_inherited_popup_menu() {
  terminal.set_type("text");
  drain();

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });
  auto panel = make_component<Panel>();
  frame->add(panel);

  auto menu = make_component<PopupMenu>();
  menu->add(make_component<MenuItem>("Panel command"));
  panel->set_component_popup_menu(menu);

  auto child = make_component<Panel>();
  // A component without mouse events never becomes the trigger's target (in
  // Swing every component listens for the mouse; in the port a component
  // asks for the events it wants, which turning inheritance on does for
  // itself as Swing's setInheritsPopupMenu does). The listener keeps the
  // child on the mouse's path for the not-inheriting case below, and the
  // preferred size gives the layout a box to place it in.
  child->add_listener([](MousePressEvent &) {
  });
  child->set_preferred_size(Dimension { 12, 3 });
  panel->add(child);
  frame->set_visible(true);
  drain();

  // The press's point, one cell inside `c` (window-local, the way the
  // terminal reports it).
  auto press_inside = [&frame](std::shared_ptr<Component> const &c) {
    auto at = c->get_location_on_screen();
    auto local = convert_point_from_screen(Point { at.x + 1, at.y + 1 }, frame);
    right_press(frame, local.x, local.y);
  };

  // The panel itself: the trigger opens its menu (the panel fills the frame,
  // so any point outside the child is on it).
  right_press(frame, 1, frame->get_height() - 2);
  CHECK(menu->is_popup_showing());
  CHECK(menu->get_invoker() == panel);

  // The child: not inheriting by default, so the trigger opens nothing; told
  // to inherit, the same trigger opens the panel's menu on the child.
  CHECK(not child->get_inherits_popup_menu());
  press_inside(child);
  CHECK(not menu->is_popup_showing());

  child->set_inherits_popup_menu(true);
  CHECK(child->get_inherits_popup_menu());
  press_inside(child);
  CHECK(menu->is_popup_showing());
  CHECK(menu->get_invoker() == child);

  frame->set_visible(false);
  drain();
}

void test_ContextMenu() {
  std::fprintf(stderr, "test_ContextMenu: component context menus and the standard text popup\n");
  test_standard_text_popup();
  test_standard_text_area_popup();
  test_popup_trigger_key_and_custom_menu();
  test_inherited_popup_menu();
  std::fprintf(stderr, "test_ContextMenu: ok\n");
}
