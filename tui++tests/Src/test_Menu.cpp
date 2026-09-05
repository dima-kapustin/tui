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
#include <tui++/RootPane.h>
#include <tui++/Screen.h>

#include <tui++/terminal/Terminal.h>

#include <cstdio>
#include <cstdlib>

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
  auto loc = file_menu->get_location_on_screen();
  auto sz = file_menu->get_size();
  auto center = Point { loc.x + sz.width / 2, loc.y + sz.height / 2 };
  auto local = convert_point_from_screen(center, frame);
  CHECK(hover(frame, local));
  CHECK(file_menu->is_armed());

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

  std::printf("PASS menu popup show/hide, hover and hit-test\n");
}
