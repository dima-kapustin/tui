// Exercises the popup menu machinery on the text screen: a top-level menu
// shows and hides its popup window, hit-testing prefers the popup over the
// frame beneath it, and a menu item fires its action. Guards the popup
// show/hide plumbing (Popup::show/hide, PopupMenu::set_visible) and the
// topmost-window hit test.
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/RootPane.h>
#include <tui++/Screen.h>

#include <tui++/terminal/Terminal.h>

#include <cassert>
#include <cstdio>

using namespace tui;

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
  frame->set_visible(true);

  assert(file_menu->is_top_level_menu());
  assert(not file_menu->is_popup_menu_visible());

  // Opening the menu shows a popup window on the screen.
  file_menu->set_popup_menu_visible(true);
  assert(file_menu->is_popup_menu_visible());

  // The popup window covers the area below the menu; hit-testing must prefer
  // it over the frame underneath.
  auto popup_menu = file_menu->get_popup_menu();
  auto pm_preferred = popup_menu->get_preferred_size();
  auto rp = get_root_pane(popup_menu);
  auto cp = rp->get_content_pane();
  std::fprintf(stderr, "popup menu preferred: %dx%d; content pane preferred: %dx%d layout=%p; root pane preferred: %dx%d\n", //
      pm_preferred.width, pm_preferred.height, cp->get_preferred_size().width, cp->get_preferred_size().height, cp->get_layout().get(), rp->get_preferred_size().width, rp->get_preferred_size().height);
  auto popup_origin = file_menu->get_location_on_screen();
  auto popup_point = Point { popup_origin.x, popup_origin.y + 1 };
  auto popup_window = screen.get_window_at(popup_point);
  if (auto w = file_menu->get_popup_menu()->get_containing_window()) {
    std::fprintf(stderr, "popup menu location on screen: %d,%d; window bounds: %d,%d %dx%d; probe: %d,%d\n", //
        popup_origin.x, popup_origin.y, w->get_x(), w->get_y(), w->get_width(), w->get_height(), popup_point.x, popup_point.y);
  }
  assert(popup_window);
  assert(popup_window.get() != frame.get());

  // The item's action fires; the click handler also closes the popup.
  std::fprintf(stderr, "item enabled=%d model=%p\n", int(item->is_enabled()), item->get_model().get());
  item->do_click();
  std::fprintf(stderr, "after do_click: fired=%d\n", fired);
  assert(fired == 1);
  file_menu->set_popup_menu_visible(false);
  assert(not file_menu->is_popup_menu_visible());

  // With the popup gone, the frame is the topmost window again.
  auto again = screen.get_window_at(popup_point);
  assert(again.get() == frame.get());

  std::printf("PASS menu popup show/hide and hit-test\n");
}
