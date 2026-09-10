// Exercises the popup menu machinery on the text screen: a top-level menu
// shows and hides its popup window, hit-testing prefers the popup over the
// frame beneath it, a menu item fires its action, and hovering/clicking a
// menu arms it and opens its popup through the window's normal mouse dispatch.
//
// The whole project (and therefore this test) is usually built with NDEBUG
// (Release), which would compile the assert()s out and make the test pass
// vacuously; CHECK below aborts regardless, so the test always verifies.
#include <tui++/Frame.h>
#include <tui++/Graphics.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/RootPane.h>
#include <tui++/Screen.h>

#include <tui++/event/FocusEvent.h>

#include <tui++/ButtonGroup.h>
#include <tui++/CheckBoxMenuItem.h>
#include <tui++/RadioButtonMenuItem.h>
#include <tui++/Char.h>
#include <tui++/event/Event.h>
#include <tui++/event/InputEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace tui;

#define CHECK(cond)                                                                                                    \
  do {                                                                                                                 \
    if (not(cond)) {                                                                                                   \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
      std::abort();                                                                                                    \
    }                                                                                                                  \
  } while (0)

// Drains the event queue. Repainting is not a side effect of the loop: a
// repaint() request accumulates damaged regions and posts a single repaint
// invocation onto the queue (Screen::add_damage). These tests drive dispatch
// manually, so pending invocations queued before the event under test (e.g.
// by set_visible) must be drained first -- otherwise pop() would return the
// stale invocation and the mouse event under test would be dropped with the
// drain, silently un-exercising the code path. Trailing invocations posted
// by the event under test are drained the same way to keep the queue aligned
// for the next event.
static void drain() {
  while (screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
  }
}

// Dispatches one mouse event through the window's normal dispatch path, the
// same way Screen::dispatch_event does after the event loop pops it.
static void dispatch_mouse(std::shared_ptr<Component> const &target, std::shared_ptr<Event> const &event) {
  target->dispatch_event(*event);
  drain();
}

// Counts MOUSE_MOVED events, standing in for the demo's hover tracker.
class MoveCounter: public EventListener<Event> {
public:
  int moved = 0;
  void event_dispatched(Event &e) override {
    if (e.id == MouseMoveEvent::MOUSE_MOVED) {
      this->moved += 1;
    }
  }
};

// Posts and dispatches a MOUSE_MOVED event at `local` (target-local), then
// returns whether the File menu (the menu bar's first child) is armed.
static bool hover(std::shared_ptr<Frame> const &frame, Point const &local) {
  drain();
  screen.post<MouseMoveEvent>(frame, InputEvent::NO_MODIFIERS, local.x, local.y);
  auto event = screen.get_event_queue().pop();
  dispatch_mouse(frame, event);
  auto menu_bar = frame->get_menu_bar();
  return menu_bar and menu_bar->get_component_count() > 0 and //
      std::dynamic_pointer_cast<MenuItem>(menu_bar->get_component(0))->is_armed();
}

void test_Menu() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });

  auto menu_bar = make_component<MenuBar>();
  auto file_menu = make_component<Menu>("File");
  auto fired = 0;
  auto item = make_component<MenuItem>("Dump");
  item->add_listener([&fired](ActionEvent &e) {
    ++fired;
  });
  file_menu->add(item);
  menu_bar->add(file_menu);
  frame->set_menu_bar(menu_bar);

  // Demo-style click-to-toggle on the top-level menu.
  file_menu->add_listener([file_menu](MousePressEvent &e) {
    if (e.id == MousePressEvent::MOUSE_RELEASED) {
      file_menu->set_popup_menu_visible(not file_menu->is_popup_menu_visible());
      e.consume();
    }
  });

  frame->set_visible(true);
  drain();

  CHECK(file_menu->is_top_level_menu());
  CHECK(not file_menu->is_popup_menu_visible());

  // Component identification (virtual to_string): a menu item identifies
  // itself by its label, an explicit name wins over it, and unnamed
  // components carry their per-instance id.
  {
    CHECK(file_menu->to_string() == "tui::Menu(File)");
    CHECK(item->to_string() == "tui::MenuItem(Dump)");
    auto frame_tag = frame->to_string();
    CHECK(frame_tag.rfind("tui::Frame(id=", 0) == 0);
    auto bar_tag = menu_bar->to_string();
    CHECK(bar_tag.rfind("tui::MenuBar(id=", 0) == 0);
    frame->set_name("window");
    CHECK(frame->to_string() == "tui::Frame(window)");
    frame->set_name("");
    CHECK(frame->to_string() == frame_tag);
  }

  // Hovering the File menu arms it (the Swing rollover/hover highlight).
  // Arming/hovering a top-level menu must not change the frame's layout: a
  // hover repaint used to destabilize the menu bar's reported size and
  // briefly shrink the frame. Lock the frame (and content pane) size here.
  auto frame_size = frame->get_size();
  auto content_size = frame->get_content_pane()->get_size();

  auto loc = file_menu->get_location_on_screen();
  auto sz = file_menu->get_size();
  auto center = Point { loc.x + sz.width / 2, loc.y + sz.height / 2 };
  auto local = convert_point_from_screen(center, frame);
  CHECK(hover(frame, local));
  CHECK(file_menu->is_armed());
  CHECK(frame->get_size() == frame_size);
  CHECK(frame->get_content_pane()->get_size() == content_size);

  // A click (press + release) opens the popup.
  drain();
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, local.x, local.y, false);
  dispatch_mouse(frame, screen.get_event_queue().pop());
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, local.x, local.y, false);
  dispatch_mouse(frame, screen.get_event_queue().pop());
  CHECK(file_menu->is_popup_menu_visible());

  // Opening the menu shows a popup window on the screen.
  file_menu->set_popup_menu_visible(true);
  drain();
  CHECK(file_menu->is_popup_menu_visible());

  // The popup window covers the area below the menu; hit-testing must prefer
  // it over the frame underneath.
  auto popup_menu = file_menu->get_popup_menu();

  // The popup menu paints the theme's menu background/foreground (Swing's
  // "PopupMenu.background/foreground"), so its items are readable on a solid
  // menu surface instead of floating over the frame beneath it.
  CHECK(popup_menu->get_background_color().has_value());
  CHECK(popup_menu->get_foreground_color().has_value());

  auto rp = get_root_pane(popup_menu);
  auto cp = rp->get_content_pane();
  auto popup_origin = file_menu->get_location_on_screen();
  auto popup_point = Point { popup_origin.x, popup_origin.y + 1 };
  auto popup_window = screen.get_window_at(popup_point);
  CHECK(popup_window);
  CHECK(popup_window.get() != frame.get());

  // Hovering a popup item arms it (the rollover highlight inside the popup).
  {
    auto item_loc = item->get_location_on_screen();
    auto item_sz = item->get_size();
    auto item_center = Point { item_loc.x + item_sz.width / 2, item_loc.y + item_sz.height / 2 };
    auto item_window = file_menu->get_popup_menu()->get_containing_window();
    auto item_local = convert_point_from_screen(item_center, item_window);
    drain();
    screen.post<MouseMoveEvent>(item_window, InputEvent::NO_MODIFIERS, item_local.x, item_local.y);
    dispatch_mouse(item_window, screen.get_event_queue().pop());
    CHECK(item->is_armed());
  }

  // The item's action fires; the click handler also closes the popup.
  item->do_click();
  CHECK(fired == 1);
  file_menu->set_popup_menu_visible(false);
  drain();
  CHECK(not file_menu->is_popup_menu_visible());

  // With the popup gone, the frame is the topmost window again.
  auto again = screen.get_window_at(popup_point);
  CHECK(again.get() == frame.get());

  // Regression: hiding a popup while its mouse dispatcher is registered as a
  // screen listener (the pointer was inside the popup) must not leave a
  // listener pointing at the destroyed popup window. Re-hovering the menu bar
  // afterwards used to crash inside event_dispatched() on the dead window.
  {
    file_menu->set_popup_menu_visible(true);
    drain();
    auto pwin = file_menu->get_popup_menu()->get_containing_window();
    auto ploc = item->get_location_on_screen();
    auto psz = item->get_size();
    auto pcenter = Point { ploc.x + psz.width / 2, ploc.y + psz.height / 2 };
    auto plocal = convert_point_from_screen(pcenter, pwin);
    drain();
    screen.post<MouseMoveEvent>(pwin, InputEvent::NO_MODIFIERS, plocal.x, plocal.y);
    dispatch_mouse(pwin, screen.get_event_queue().pop());
    CHECK(item->is_armed());

    file_menu->set_popup_menu_visible(false); // hides and destroys the popup window
    drain();
    CHECK(hover(frame, local));
  }

  // Regression: moving over a dead zone of the window (no component accepts
  // mouse events there, e.g. an empty panel area) must still reach screen
  // listeners -- the hover panel shows every movement, not just the moves
  // over interactive widgets.
  {
    auto counter = std::make_shared<MoveCounter>();
    screen.add_listener(EventType::MOUSE_MOVE, counter);
    auto dead = Point { 40, 12 }; // below the menu bar, empty content area
    drain();
    screen.post<MouseMoveEvent>(frame, InputEvent::NO_MODIFIERS, dead.x, dead.y);
    dispatch_mouse(frame, screen.get_event_queue().pop());
    CHECK(counter->moved == 1);
    screen.remove_listener(counter);
  }

  // Regression: a terminal resize must not stretch a popup window to the
  // screen size. The top-level frame tracks the screen, but the popup keeps
  // its own size and position; stretching it paints the open menu (and its
  // selection highlight) across the whole window, covering the frame.
  {
    file_menu->set_popup_menu_visible(true);
    drain();
    auto pwin = file_menu->get_popup_menu()->get_containing_window();
    CHECK(pwin);
    auto popup_size = pwin->get_size();
    auto popup_loc = pwin->get_location();
    CHECK(popup_size.width > 0 and popup_size.width < frame->get_width());

    screen.resized();
    drain();

    CHECK(frame->get_size() == screen.get_size());
    CHECK(pwin->get_size() == popup_size);
    CHECK(pwin->get_location() == popup_loc);

    file_menu->set_popup_menu_visible(false);
    drain();
    CHECK(not file_menu->is_popup_menu_visible());
  }

  // Regression: clip_rect must intersect with the CURRENT clip. The old code
  // measured the clip's size from the already-clipped edge (bottom =
  // clip_top + clip.height), extending the clip past the screen bottom for a
  // rect that starts inside the clip: the row of a popup that straddles the
  // screen's bottom edge then painted out of bounds (an abort in debug
  // builds, memory corruption in release ones).
  {
    auto g = screen.get_graphics(); // clip = the whole screen
    g->clip_rect(0, 1, 10, 23);     // rows 1..23
    g->clip_rect(0, 20, 10, 8);     // starts inside, extends past the bottom
    auto clip = g->get_clip_rect();
    CHECK(clip.y == 20);
    CHECK(clip.height == 4);        // rows 20..23; the old code kept 8
  }

  // Take the frame off the screen: the windows of shown frames stay alive in
  // the screen's window list, and the screen tests that follow (byte-exact
  // emission measurements) must run on an empty screen.
  frame->set_visible(false);
  drain();

  std::printf("PASS menu popup show/hide, hover and hit-test\n");
}

// Posts one key event to `frame` and dispatches it through the window's
// normal dispatch path (Window::dispatch_event), the way the event loop
// dispatches a key the terminal posted to the focused window. Returns the
// event, whose `consumed` flag says whether the menu system took the key.
static std::shared_ptr<Event> dispatch_key(std::shared_ptr<Frame> const &frame, KeyEvent::Type type, KeyEvent::KeyCode code, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(frame, type, code, modifiers);
  auto event = screen.get_event_queue().pop();
  frame->dispatch_event(*event);
  drain();
  return event;
}

static std::shared_ptr<Event> dispatch_char(std::shared_ptr<Frame> const &frame, Char c, InputEvent::Modifiers modifiers = InputEvent::NO_MODIFIERS) {
  drain();
  screen.post<KeyEvent>(frame, c, modifiers);
  auto event = screen.get_event_queue().pop();
  frame->dispatch_event(*event);
  drain();
  return event;
}

// Exercises the keyboard navigation of a menu bar (MenuKeyboardManager): F10
// and Alt+mnemonic put the bar on the keyboard, arrows move the highlight
// across the top-level menus and through the items of an open popup, Enter
// activates the armed item (dismissing the popup through the selection path),
// Escape steps back out, and mnemonic letters select their items. Keys the
// menu system does not want fall through unconsumed.
void test_MenuKeyboard() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 100, 30 });

  auto menu_bar = make_component<MenuBar>();
  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  auto edit_menu = make_component<Menu>("Edit");
  edit_menu->set_mnemonic('E');
  menu_bar->add(file_menu);
  menu_bar->add(edit_menu);
  frame->set_menu_bar(menu_bar);

  struct Counters {
    int new_file = 0;
    int open = 0;
    int cut = 0;
    int paste = 0;
  };
  auto counters = std::make_shared<Counters>();

  auto new_item = make_component<MenuItem>("New", Char { 'N' });
  new_item->add_listener([counters](ActionEvent &) {
    ++counters->new_file;
  });
  auto open_item = make_component<MenuItem>("Open", Char { 'O' });
  open_item->add_listener([counters](ActionEvent &) {
    ++counters->open;
  });
  auto save_item = make_component<MenuItem>("Save", Char { 'S' });
  save_item->set_enabled(false); // navigation must skip it
  auto cut_item = make_component<MenuItem>("Cut", Char { 't' });
  cut_item->add_listener([counters](ActionEvent &) {
    ++counters->cut;
  });
  auto paste_item = make_component<MenuItem>("Paste", Char { 'P' });
  paste_item->add_listener([counters](ActionEvent &) {
    ++counters->paste;
  });

  // File: New, (separator), Open, Save (disabled), (separator)
  file_menu->add(new_item);
  file_menu->add_separator();
  file_menu->add(open_item);
  file_menu->add(save_item);
  file_menu->add_separator();
  // Edit: Cut, Copy, Paste
  edit_menu->add(cut_item);
  edit_menu->add(make_component<MenuItem>("Copy", Char { 'C' }));
  edit_menu->add(paste_item);

  frame->set_visible(true);
  drain();

  // The mnemonic letters of the top-level menus are drawn bold with a single
  // underline ("File", "Edit"), so the Alt+letter shortcuts stand out: the
  // full paint of the menu bar row must carry the bold + underline SGR
  // (1;4) at the mnemonic cells.
  {
    auto capture = std::ostringstream { };
    auto *old_cout = std::cout.rdbuf(capture.rdbuf());
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    std::cout.rdbuf(old_cout);
    auto bytes = capture.str();
    CHECK(bytes.find("\x1b[1;4m") != std::string::npos);
  }

  auto armed = [](std::shared_ptr<Menu> const &menu) {
    return menu->is_armed();
  };

  // Without a keyboard session the navigation keys pass through untouched.
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT)->consumed);
  CHECK(not dispatch_char(frame, Char { 'q' })->consumed);
  CHECK(not dispatch_char(frame, Char { 'q' }, InputEvent::ALT_DOWN)->consumed); // no menu with mnemonic 'q'
  CHECK(not armed(file_menu));
  CHECK(not armed(edit_menu));

  // F10 arms the first top-level menu; F10 again leaves the bar.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(armed(file_menu));
  CHECK(not armed(edit_menu));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(not armed(file_menu));

  // Left/right move the armed highlight across the bar, without wrap-around.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT)->consumed);
  CHECK(not armed(file_menu));
  CHECK(armed(edit_menu));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT)->consumed);
  CHECK(armed(edit_menu)); // last menu: the highlight stays
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT)->consumed);
  CHECK(armed(file_menu));
  CHECK(not armed(edit_menu));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT)->consumed);
  CHECK(armed(file_menu)); // first menu: the highlight stays

  // Down opens the popup and arms its first item; the arrows then move
  // through the enabled items only, skipping separators and the disabled
  // Save, and wrap around at the ends.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  CHECK(new_item->is_armed());
  CHECK(not open_item->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(not new_item->is_armed());
  CHECK(open_item->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(new_item->is_armed()); // wraps around
  CHECK(not open_item->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_UP)->consumed);
  CHECK(open_item->is_armed()); // ... and back up

  // A mnemonic letter selects the item it stands for; letters no item
  // claims are swallowed (the open popup is modal) but change nothing.
  auto typed = dispatch_char(frame, Char { 'o' });
  CHECK(typed->consumed);
  CHECK(open_item->is_armed());
  typed = dispatch_char(frame, Char { 'x' });
  CHECK(typed->consumed);
  CHECK(open_item->is_armed());
  CHECK(dispatch_char(frame, Char { 'n' })->consumed);
  CHECK(new_item->is_armed());

  // Enter activates the armed item: its action runs, the popup closes
  // through the selection path, the bar highlight goes off and the
  // keyboard leaves the menu entirely.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(counters->new_file == 1);
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(not armed(file_menu));
  CHECK(not armed(edit_menu));
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);

  // Alt+mnemonic opens the popup of the menu it stands for (no item armed
  // yet); the first arrow then picks an item.
  CHECK(dispatch_char(frame, Char { 'e' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(armed(edit_menu));
  CHECK(not armed(file_menu));
  CHECK(edit_menu->is_popup_menu_visible());
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(not cut_item->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(cut_item->is_armed());

  // With a popup open, Alt+mnemonic hops to the other top-level menu's
  // popup (and back); plain letters keep working for the items.
  CHECK(dispatch_char(frame, Char { 'f' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(not armed(edit_menu));
  CHECK(armed(file_menu));
  CHECK(file_menu->is_popup_menu_visible());
  CHECK(not edit_menu->is_popup_menu_visible());
  CHECK(dispatch_char(frame, Char { 'e' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(edit_menu->is_popup_menu_visible());
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(dispatch_char(frame, Char { 'p' })->consumed);
  CHECK(paste_item->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(counters->paste == 1);
  CHECK(not edit_menu->is_popup_menu_visible());
  CHECK(not armed(file_menu));
  CHECK(not armed(edit_menu));

  // Left/right hop between the popups of adjacent menus; left on the first
  // menu's popup closes it and steps back onto the bar.
  CHECK(dispatch_char(frame, Char { 'e' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  CHECK(not edit_menu->is_popup_menu_visible());
  CHECK(armed(file_menu));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT)->consumed);
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(armed(file_menu)); // popup closed, the bar still holds the keyboard

  // Escape closes the open popup back onto the armed bar; the next Escape
  // leaves the bar (the keys belong to the component again).
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(armed(file_menu));
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);
  CHECK(not armed(file_menu));
  CHECK(not armed(edit_menu));
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);

  // An Escape with nothing armed falls through (the text area of the demos
  // uses it to leave its search mode, for example).
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);

  // Tab and Shift+Tab end a keyboard session the way focus traversal ends
  // menus on Windows and in Swing: with a popup open the popup closes first,
  // from the armed bar the highlight goes off, and the keys belong to the
  // component underneath again (this frame has no focusable content, so the
  // focus handover itself is exercised by test_FocusTraversal).
  CHECK(dispatch_char(frame, Char { 'f' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  CHECK(not new_item->is_armed()); // Alt+mnemonic opens the popup without arming an item
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_TAB)->consumed);
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(not armed(file_menu));
  CHECK(not armed(edit_menu));
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);

  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(armed(file_menu));
  auto shift_tab = dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_TAB, InputEvent::SHIFT_DOWN);
  CHECK(shift_tab->consumed);
  CHECK(not armed(file_menu));
  CHECK(not armed(edit_menu));
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT)->consumed);
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(armed(file_menu));
  auto back_tab = dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_BACK_TAB);
  CHECK(back_tab->consumed);
  CHECK(not armed(file_menu));

  // A mouse press ends the keyboard session. Pressing outside the menu bar
  // also drops the armed highlight, so it does not linger after the mouse
  // takes over.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F10)->consumed);
  CHECK(armed(file_menu));
  drain();
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 40, 20, false);
  dispatch_mouse(frame, screen.get_event_queue().pop());
  CHECK(not armed(file_menu));
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);

  // While a popup is open the keyboard keeps working even after a mouse
  // press (the press only ends the F10-style bar session, not the popup's
  // own navigation).
  CHECK(dispatch_char(frame, Char { 'f' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  drain();
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, 40, 20, false);
  dispatch_mouse(frame, screen.get_event_queue().pop());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(new_item->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);
  CHECK(not file_menu->is_popup_menu_visible());

  // Pressing a top-level menu (inside the bar) keeps its armed highlight:
  // the pointer is on it, so the mouse owns the highlight now.
  auto bar_loc = file_menu->get_location_on_screen();
  auto bar_local = convert_point_from_screen(bar_loc.x + 1, bar_loc.y, frame);
  drain();
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, bar_local.x, bar_local.y, false);
  dispatch_mouse(frame, screen.get_event_queue().pop());

  // Regression: a mouse hover over an open popup must leave the hovered
  // item fully shareable. A hover rebuilds the selection path from the
  // item's raw component pointer (MenuItemUI::get_path); wrapping that raw
  // pointer in a fresh shared_ptr used to hijack the item's
  // enable_shared_from_this, so activating the item afterwards -- here with
  // the Enter shortcut, in the demos with a second hover + mouse pick --
  // threw bad_weak_ptr from the item's shared_from_this().
  CHECK(dispatch_char(frame, Char { 'e' }, InputEvent::ALT_DOWN)->consumed); // Edit popup open
  CHECK(edit_menu->is_popup_menu_visible());
  // An arrow arms Cut, so the selection path now ends at a popup item
  // (hovering rebuilds that path through get_path).
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(cut_item->is_armed());
  {
    // Hover Paste with the mouse: the hover arms it (and un-arms Cut).
    auto popup_window = edit_menu->get_popup_menu()->get_containing_window();
    auto paste_loc = paste_item->get_location_on_screen();
    auto paste_size = paste_item->get_size();
    auto paste_center = Point { paste_loc.x + paste_size.width / 2, paste_loc.y + paste_size.height / 2 };
    auto paste_local = convert_point_from_screen(paste_center, popup_window);
    drain();
    screen.post<MouseMoveEvent>(popup_window, InputEvent::NO_MODIFIERS, paste_local.x, paste_local.y);
    dispatch_mouse(popup_window, screen.get_event_queue().pop());
    CHECK(paste_item->is_armed());
    CHECK(not cut_item->is_armed());
  }
  // The Enter shortcut activates the hovered Paste: its action fires and the
  // popup closes. This used to throw bad_weak_ptr while the action fired on
  // the hovered item.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(counters->paste == 2);
  CHECK(not edit_menu->is_popup_menu_visible());
  CHECK(not armed(file_menu));
  CHECK(not armed(edit_menu));

  // Take the frame off the screen and make sure the keyboard session is
  // fully over, so later tests start clean.
  file_menu->set_popup_menu_visible(false);
  edit_menu->set_popup_menu_visible(false);
  frame->set_visible(false);
  drain();

  std::printf("PASS menu keyboard navigation (F10, arrows, Enter, Escape, mnemonics)\n");
}

// The submenu machinery: a nested Menu row of a popup opens its own popup
// (hovering it opens it, Right/Enter/Space walk into it from the keyboard),
// the radio items of the submenu stay exclusive through a ButtonGroup,
// activating an item dismisses the whole open chain, and Left/Escape step
// back out of a submenu again. The rows of the open popups also paint the
// check/radio indicators and the submenu's arrow on the text screen (the
// box-drawing glyphs ☒/☐, ●/○ and ►).
void test_MenuSubmenu() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 100, 30 });

  auto menu_bar = make_component<MenuBar>();
  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  menu_bar->add(file_menu);
  frame->set_menu_bar(menu_bar);

  // The File popup rows: two check items (one checked, one not, so the popup
  // paints both box glyphs), a separator and the Scheme submenu row.
  auto wrap = make_component<CheckBoxMenuItem>("Word wrap");
  wrap->set_selected(true);
  auto grid = make_component<CheckBoxMenuItem>("Show grid");
  file_menu->add(wrap);
  file_menu->add(grid);
  file_menu->add_separator();

  auto scheme_menu = make_component<Menu>("Scheme");
  scheme_menu->set_mnemonic('c');
  auto group = std::make_shared<ButtonGroup>();
  auto classic = make_component<RadioButtonMenuItem>("Classic", Char { 'C' });
  auto dark = make_component<RadioButtonMenuItem>("Dark", Char { 'D' });
  auto system = make_component<RadioButtonMenuItem>("System", Char { 'S' });
  group->add(classic);
  group->add(dark);
  group->add(system);
  classic->set_selected(true);
  auto picks = std::vector<std::string> { };
  for (auto &&item : { classic, dark, system }) {
    auto label = item->get_text();
    item->add_listener([&picks, label](ActionEvent &) {
      picks.push_back(label);
    });
  }
  scheme_menu->add(classic);
  scheme_menu->add(dark);
  scheme_menu->add(system);
  file_menu->add(scheme_menu);

  // Demo-style click-to-toggle on the top-level menu.
  file_menu->add_listener([file_menu](MousePressEvent &e) {
    if (e.id == MousePressEvent::MOUSE_RELEASED) {
      file_menu->set_popup_menu_visible(not file_menu->is_popup_menu_visible());
      e.consume();
    }
  });

  frame->set_visible(true);
  drain();

  CHECK(file_menu->is_top_level_menu());
  CHECK(not scheme_menu->is_top_level_menu()); // a row of the File popup
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(not scheme_menu->is_popup_menu_visible());

  // ---- the open popups paint the indicator glyphs on the text screen ----
  // Word wrap carries the checked box (☒), Show grid the empty one (☐), the
  // selected Classic the filled radio circle (●), Dark/System the hollow one
  // (○), and the Scheme row the submenu arrow (►).
  {
    file_menu->set_popup_menu_visible(true);
    scheme_menu->set_popup_menu_visible(true);
    drain();

    auto capture = std::ostringstream { };
    auto *old_cout = std::cout.rdbuf(capture.rdbuf());
    dynamic_cast<TextScreen&>(screen).clear();
    screen.refresh();
    std::cout.rdbuf(old_cout);
    auto bytes = capture.str();
    CHECK(bytes.find("\xe2\x98\x92") != std::string::npos); // U+2611 '☒'
    CHECK(bytes.find("\xe2\x98\x90") != std::string::npos); // U+2610 '☐'
    CHECK(bytes.find("\xe2\x97\x8f") != std::string::npos); // U+25CF '●'
    CHECK(bytes.find("\xe2\x97\x8b") != std::string::npos); // U+25CB '○'
    CHECK(bytes.find("\xe2\x8f\xb5") != std::string::npos); // U+25BA '⏵'
    file_menu->set_popup_menu_visible(false);
    scheme_menu->set_popup_menu_visible(false);
    drain();
  }

  // ---- keyboard: Right on the armed submenu row walks into it, letters
  // select the radio items, Enter activates the armed one and closes the
  // whole chain ----
  CHECK(dispatch_char(frame, Char { 'f' }, InputEvent::ALT_DOWN)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  CHECK(not scheme_menu->is_popup_menu_visible());
  // The mnemonic 'c' arms the Scheme row (a letter only selects; opening is
  // a key of its own).
  CHECK(dispatch_char(frame, Char { 'c' })->consumed);
  CHECK(scheme_menu->is_armed());
  // Right opens the armed row's submenu and arms its first radio item.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_RIGHT)->consumed);
  CHECK(file_menu->is_popup_menu_visible());
  CHECK(scheme_menu->is_popup_menu_visible());
  CHECK(classic->is_armed());
  // The arrows move through the submenu's items; a letter jumps to its
  // mnemonic item.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(dark->is_armed());
  CHECK(not classic->is_armed());
  CHECK(dispatch_char(frame, Char { 's' })->consumed);
  CHECK(system->is_armed());
  CHECK(not dark->is_armed());
  // Enter activates the armed radio item: its action fires, the radio group
  // moves to it, and the whole popup chain closes -- the bar un-arms and the
  // keyboard is free again.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(picks == (std::vector<std::string> { "System" }));
  CHECK(system->is_selected());
  CHECK(not classic->is_selected());
  CHECK(not dark->is_selected());
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(not scheme_menu->is_popup_menu_visible());
  CHECK(not file_menu->is_armed());
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);

  // ---- keyboard: Enter and Space on the armed row open it too; Left and
  // Escape step back out of the chain ----
  CHECK(dispatch_char(frame, Char { 'f' }, InputEvent::ALT_DOWN)->consumed);
  // Down wraps through the File popup's items: the two check items, then the
  // submenu row.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(wrap->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(grid->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(scheme_menu->is_armed());
  // Enter on the armed submenu row opens it (like Right).
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ENTER)->consumed);
  CHECK(scheme_menu->is_popup_menu_visible());
  CHECK(classic->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);
  CHECK(dark->is_armed());
  // Left steps back onto the submenu's row; the File popup stays open.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_LEFT)->consumed);
  CHECK(not scheme_menu->is_popup_menu_visible());
  CHECK(scheme_menu->is_armed());
  CHECK(file_menu->is_popup_menu_visible());
  // Escape closes the File popup back onto the armed bar; a second Escape
  // leaves the bar.
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(file_menu->is_armed());
  CHECK(dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_ESCAPE)->consumed);
  CHECK(not file_menu->is_armed());
  CHECK(not dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_DOWN)->consumed);

  // ---- mouse: hovering the submenu row of an open popup opens its popup, a
  // click on a radio row picks it, and closing the top-level popup takes the
  // whole chain down with it ----
  auto click_at = [](std::shared_ptr<Window> const &window, Point const &local) {
    drain();
    screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, local.x, local.y, false);
    dispatch_mouse(window, screen.get_event_queue().pop());
    screen.post<MousePressEvent>(window, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, local.x, local.y, false);
    dispatch_mouse(window, screen.get_event_queue().pop());
  };
  auto hover_at = [](std::shared_ptr<Window> const &window, Point const &local) {
    drain();
    screen.post<MouseMoveEvent>(window, InputEvent::NO_MODIFIERS, local.x, local.y);
    dispatch_mouse(window, screen.get_event_queue().pop());
  };

  // Hover and click the File menu row (demo-style toggle, see above).
  {
    auto loc = file_menu->get_location_on_screen();
    auto local = convert_point_from_screen(Point { loc.x + file_menu->get_width() / 2, loc.y + file_menu->get_height() / 2 }, frame);
    hover_at(frame, local);
    CHECK(file_menu->is_armed());
    click_at(frame, local);
    CHECK(file_menu->is_popup_menu_visible());
  }

  // Hover the Scheme row of the open popup: the hover opens the submenu.
  {
    auto file_popup = file_menu->get_popup_menu()->get_containing_window();
    auto row_loc = scheme_menu->get_location_on_screen();
    auto row_local = convert_point_from_screen(Point { row_loc.x + scheme_menu->get_width() / 2, row_loc.y + scheme_menu->get_height() / 2 }, file_popup);
    hover_at(file_popup, row_local);
    CHECK(scheme_menu->is_armed());
    CHECK(scheme_menu->is_popup_menu_visible());
  }

  // Click Dark in the submenu: its action fires and the radio group moves to
  // it. The popup chain was opened directly (demo style), so it stays up.
  {
    auto submenu_popup = scheme_menu->get_popup_menu()->get_containing_window();
    auto dark_loc = dark->get_location_on_screen();
    auto dark_local = convert_point_from_screen(Point { dark_loc.x + dark->get_width() / 2, dark_loc.y + dark->get_height() / 2 }, submenu_popup);
    click_at(submenu_popup, dark_local);
    CHECK(picks == (std::vector<std::string> { "System", "Dark" }));
    CHECK(dark->is_selected());
    CHECK(not classic->is_selected());
    CHECK(not system->is_selected());
    CHECK(file_menu->is_popup_menu_visible());
    CHECK(scheme_menu->is_popup_menu_visible());
  }

  // Closing the top-level popup (as a pick's action does) takes the open
  // submenu down with it.
  file_menu->set_popup_menu_visible(false);
  drain();
  CHECK(not file_menu->is_popup_menu_visible());
  CHECK(not scheme_menu->is_popup_menu_visible());

  // Reopen and click a check item of the File popup: the pick toggles it and
  // the row keeps its highlight (the pick did not dismiss the popup, so the
  // row under the pointer stays armed -- the click's press animation must not
  // leave it looking unselected).
  {
    auto loc = file_menu->get_location_on_screen();
    auto local = convert_point_from_screen(Point { loc.x + file_menu->get_width() / 2, loc.y + file_menu->get_height() / 2 }, frame);
    click_at(frame, local);
    CHECK(file_menu->is_popup_menu_visible());

    auto file_popup = file_menu->get_popup_menu()->get_containing_window();
    auto wrap_loc = wrap->get_location_on_screen();
    auto wrap_local = convert_point_from_screen(Point { wrap_loc.x + wrap->get_width() / 2, wrap_loc.y + wrap->get_height() / 2 }, file_popup);
    click_at(file_popup, wrap_local);
    CHECK(not wrap->is_selected()); // the pick toggled the check off
    CHECK(file_menu->is_popup_menu_visible());
    CHECK(wrap->is_armed());

    // A second pick toggles it back on, the highlight still in place.
    click_at(file_popup, wrap_local);
    CHECK(wrap->is_selected());
    CHECK(wrap->is_armed());

    // The row that loses the pointer loses the highlight with it (the popup
    // stays open).
    auto grid_loc = grid->get_location_on_screen();
    auto grid_local = convert_point_from_screen(Point { grid_loc.x + grid->get_width() / 2, grid_loc.y + grid->get_height() / 2 }, file_popup);
    hover_at(file_popup, grid_local);
    CHECK(grid->is_armed());
    CHECK(not wrap->is_armed());
  }

  file_menu->set_popup_menu_visible(false);
  frame->set_visible(false);
  drain();

  std::printf("PASS menu submenus (indicator glyphs, hover and keyboard chains)\n");
}

// The items' accelerators fire from anywhere in the window: the stroke enters
// the KeyboardManager's registry through the item's WHEN_IN_FOCUSED_WINDOW
// input map, and the window offers it to the item after the focused
// component's own bindings. A changed accelerator moves the binding, and an
// item that leaves the window stops answering to it.
void test_MenuAccelerators() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });

  auto menu_bar = make_component<MenuBar>();
  auto file_menu = make_component<Menu>("File");
  menu_bar->add(file_menu);

  auto opens = 0, closes = 0;
  auto open_item = make_component<MenuItem>("Open");
  open_item->add_listener([&opens](ActionEvent &) {
    ++opens;
  });
  open_item->set_accelerator(KeyStroke { KeyEvent::VK_O, InputEvent::CTRL_DOWN });
  auto close_item = make_component<MenuItem>("Close");
  close_item->add_listener([&closes](ActionEvent &) {
    ++closes;
  });
  close_item->set_accelerator(KeyStroke { KeyEvent::VK_W, InputEvent::CTRL_DOWN });
  file_menu->add(open_item);
  file_menu->add(close_item);
  frame->set_menu_bar(menu_bar);

  // A content component owns the focus, so the keys arrive the way a real
  // window delivers them.
  auto content = make_component<Panel>();
  frame->get_content_pane()->add(content);
  frame->set_visible(true);
  drain();
  content->request_focus(FocusEvent::Cause::ACTIVATION);
  CHECK(content->is_focus_owner());

  // Each item answers to its own stroke, and to no other.
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_O, InputEvent::CTRL_DOWN);
  CHECK(opens == 1);
  CHECK(closes == 0);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_W, InputEvent::CTRL_DOWN);
  CHECK(closes == 1);
  CHECK(opens == 1);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_Q, InputEvent::CTRL_DOWN);
  CHECK(opens == 1 and closes == 1);

  // A changed accelerator moves the binding: the stroke the item had falls
  // silent and the new one fires.
  open_item->set_accelerator(KeyStroke { KeyEvent::VK_T, InputEvent::CTRL_DOWN });
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_O, InputEvent::CTRL_DOWN);
  CHECK(opens == 1);
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_T, InputEvent::CTRL_DOWN);
  CHECK(opens == 2);

  // An item that leaves the window stops answering to its stroke.
  file_menu->remove(open_item);
  drain();
  dispatch_key(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_T, InputEvent::CTRL_DOWN);
  CHECK(opens == 2);

  frame->set_visible(false);
  drain();

  std::printf("PASS menu accelerators (window-wide strokes, live changes)\n");
}
