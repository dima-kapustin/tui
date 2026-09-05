// Menu hover probe for the sixel screen.
//
// Reproduces the MenuBarDemo interaction sequence -- click a top-level menu,
// hover several popup items, leave the popup -- by driving the REAL input
// path (Terminal::new_mouse_event / new_mouse_move_event) into the REAL event
// loop, and checks the framebuffer after every step:
//
//   * which menu items are armed,
//   * how many pixels of the armed highlight color remain inside each item's
//     bounds (they must all be gone once the pointer leaves the item).
//
// The library event log (stderr) shows the repaint regions and the tiles
// actually flushed, so a missing erase shows up as both leftover pixels and
// an absent repaint region.
//
// Build (from the repo root):
//   g++ -g -std=gnu++26 -DUNICODE -Itui++/Inc tools/probe_menu_hover.cpp \
//       build/tui++/libtui++.a -lkernel32 -luser32 -lgdi32 -lwinspool \
//       -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 \
//       -o tools/probe_menu_hover.exe
//
// Run (output may be redirected; the probe only inspects the framebuffer):
//   tools/probe_menu_hover.exe 2> hover_trace.log
//
// The raw terminal output (cursor moves + sixel images) is captured to
// probe_stream.bin (stdout is redirected at startup); tools/render_sixel.py
// decodes it back into pixels for stream-level verification.

#include <tui++/Event.h>
#include <tui++/BorderLayout.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>
#include <tui++/border/EmptyBorder.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/sixel/SixelScreen.h>
#include <tui++/util/log.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <thread>
#include <vector>

using namespace tui;

namespace {

void sleep_ms(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct RGB {
  uint8_t r, g, b;
  bool operator==(RGB const &o) const {
    return r == o.r and g == o.g and b == o.b;
  }
};

// The framebuffer probe accessor (SixelScreen::debug_pixels) did not exist
// in every historical build this probe runs against; -DPROBE_NO_FB builds
// the stream-capture-only variant for those.
#ifndef PROBE_NO_FB

RGB px_at(SixelScreen &gs, int x, int y) {
  auto const *p = gs.debug_pixels() + (std::size_t(y) * gs.get_pixel_width() + x) * 3;
  return { p[0], p[1], p[2] };
}

int count_color(SixelScreen &gs, Rectangle const &rect, RGB color) {
  auto n = 0;
  for (auto y = rect.y; y < rect.bottom(); ++y) {
    for (auto x = rect.x; x < rect.right(); ++x) {
      if (px_at(gs, x, y) == color) {
        ++n;
      }
    }
  }
  return n;
}

#endif

Rectangle item_rect(std::shared_ptr<MenuItem> const &item) {
  auto p = convert_point_to_screen(0, 0, item);
  return { p.x, p.y, item->get_width(), item->get_height() };
}

struct Probe {
  std::shared_ptr<Menu> file_menu;
  std::shared_ptr<MenuItem> item_new;
  std::shared_ptr<MenuItem> item_open;
  std::shared_ptr<MenuItem> item_save;
  std::shared_ptr<MenuItem> item_exit;
  std::shared_ptr<Panel> content;

  // The armed fill color sampled while an item was armed, and whether a
  // sample has been taken yet.
  RGB highlight { 0, 0, 0 };
  bool has_highlight = false;
};

// Feeds mouse events into the screen exactly the way Terminal's input parser
// does (the real methods are private): convert the terminal cell to screen
// pixels, hit-test the window, retarget to window-local coordinates, and post
// a system-generated event on the screen queue.
struct Input {
  MouseEvent::Modifiers modifiers { };

  void post(std::shared_ptr<Event> const &event) {
    screen.post(event);
  }

  void move(int x, int y) {
    auto p = screen.convert_mouse_point(x, y);
    auto window = screen.get_window_at(p);
    if (window) {
      p = convert_point_from_screen(p, window);
    }
    post(std::make_shared<MouseMoveEvent>(window, modifiers, p.x, p.y));
  }

  void press(MousePressEvent::Type type, MousePressEvent::Button button, int x, int y) {
    // Terminal::new_mouse_event carries the button in the modifiers of a
    // PRESSED (and clears it for a RELEASED): was_button_down_before() XORs
    // it back out, so this exact shape is what makes the press retarget to
    // the component under the pointer.
    auto mods = modifiers;
    if (type == MousePressEvent::MOUSE_PRESSED) {
      mods |= to_modifiers(button);
    }
    auto p = screen.convert_mouse_point(x, y);
    auto window = screen.get_window_at(p);
    if (window) {
      p = convert_point_from_screen(p, window);
    }
    post(std::make_shared<MousePressEvent>(window, type, button, mods, p.x, p.y, false));
  }

  void click(int x, int y) {
    press(MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, x, y);
    press(MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, x, y);
    // Terminal posts a MouseClickEvent after a press-release pair.
    auto p = screen.convert_mouse_point(x, y);
    auto window = screen.get_window_at(p);
    if (window) {
      p = convert_point_from_screen(p, window);
    }
    post(std::make_shared<MouseClickEvent>(window, MousePressEvent::LEFT_BUTTON, modifiers, p.x, p.y, 1, false));
  }
};

void dump(SixelScreen &gs, Probe &probe, char const *step) {
  std::printf("-- %s --\n", step);
  std::printf("  armed: File=%d", int(probe.file_menu->is_armed()));
  for (auto &&item : { probe.item_new, probe.item_open, probe.item_save, probe.item_exit }) {
    std::printf(" %s=%d", item->get_text().c_str(), int(item->is_armed()));
  }
  std::printf("\n");

  for (auto &&item : { probe.item_new, probe.item_open, probe.item_save, probe.item_exit }) {
    auto rect = item_rect(item);
#ifndef PROBE_NO_FB
    auto remaining = probe.has_highlight ? count_color(gs, rect, probe.highlight) : -1;
    // The demo frame's cyan double border must never repaint into an item:
    // a region repaint used to re-stroke the border around the damaged
    // region's own perimeter, leaving cyan frames on items the mouse left.
    auto frame_lines = count_color(gs, rect, RGB { 0, 255, 255 });
    std::printf("  %-5s rect=(%d, %d %dx%d) highlight-pixels=%d frame-pixels=%d\n", item->get_text().c_str(), rect.x, rect.y, rect.width, rect.height, remaining, frame_lines);

    // Sample the highlight fill color from the first armed item we see.
    if (not probe.has_highlight and item->is_armed()) {
      probe.highlight = px_at(gs, rect.x + rect.width / 2, rect.y + rect.height / 2);
      probe.has_highlight = true;
      std::printf("  (highlight color sampled at %s: #%02x%02x%02x)\n", item->get_text().c_str(), probe.highlight.r, probe.highlight.g, probe.highlight.b);
    }
#else
    std::printf("  %-5s rect=(%d, %d %dx%d)\n", item->get_text().c_str(), rect.x, rect.y, rect.width, rect.height);
#endif
  }
}

} // namespace

int main() {
#ifndef PROBE_NO_FB
  // Capture the exact bytes sent to the terminal (cursor moves and sixel
  // images) so the stream can be decoded and diffed against other builds.
  // (PROBE_NO_FB builds capture via shell redirection instead.)
  freopen("probe_stream.bin", "wb", stdout);
#endif

  terminal.set_type("sixel");
  util::event_log = &std::cerr;

  auto probe = Probe { };

  // The same component tree shape as MenuBarDemo (frame + menu bar + content).
  auto cell = screen.get_text_metrics()->get_line_height();
  auto frame = make_component<Frame>();
  frame->set_background_color(GREEN_COLOR);
  frame->set_size(screen.get_size());
  frame->set_name("main frame");
  frame->get_content_pane()->set_border(std::make_shared<EmptyBorder>(2 * cell, 2 * cell, 2 * cell, 2 * cell));

  probe.content = make_component<Panel>();
  probe.content->set_name("content");
  frame->add(probe.content);

  probe.file_menu = make_component<Menu>("File");
  probe.file_menu->set_name("File");
  probe.item_new = make_component<MenuItem>("New");
  probe.item_open = make_component<MenuItem>("Open");
  probe.item_save = make_component<MenuItem>("Save");
  probe.item_exit = make_component<MenuItem>("Exit");
  for (auto &&item : { probe.item_new, probe.item_open, probe.item_save, probe.item_exit }) {
    probe.file_menu->add(item);
  }

  auto edit_menu = make_component<Menu>("Edit");
  edit_menu->add(make_component<MenuItem>("Cut"));

  auto menu_bar = make_component<MenuBar>();
  menu_bar->set_name("menu bar");
  menu_bar->add(probe.file_menu);
  menu_bar->add(edit_menu);
  frame->set_menu_bar(menu_bar);

  // The demo's popup toggle: a release on the top-level menu opens the popup.
  probe.file_menu->add_listener([file_menu = probe.file_menu](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_RELEASED) {
      return;
    }
    file_menu->set_popup_menu_visible(not file_menu->is_popup_menu_visible());
    file_menu->set_armed(true);
    file_menu->repaint();
    e.consume();
  });

  frame->set_visible(true);

  std::thread loop([] {
    terminal.run_event_loop();
  });

  auto &gs = static_cast<SixelScreen &>(screen);
  auto cw = gs.get_cell_width();
  auto ch = gs.get_cell_height();

  sleep_ms(600); // initial full paint

  auto to_cell = [&](Point const &p, Dimension const &sz) {
    return Point { (p.x + sz.width / 2) / cw, (p.y + sz.height / 2) / ch };
  };
  auto input = Input { };
  auto item_cell = [&](std::shared_ptr<MenuItem> const &item) {
    auto p = convert_point_to_screen(0, 0, item);
    return to_cell(p, { item->get_width(), item->get_height() });
  };

  auto file_pos = convert_point_to_screen(0, 0, probe.file_menu);
  auto file_cell = to_cell(file_pos, { probe.file_menu->get_width(), probe.file_menu->get_height() });
  std::printf("frame %dx%d px, cell %dx%d px\n", gs.get_pixel_width(), gs.get_pixel_height(), cw, ch);
  std::printf("File menu at (%d, %d %dx%d) -> cell (%d, %d)\n", file_pos.x, file_pos.y, probe.file_menu->get_width(), probe.file_menu->get_height(), file_cell.x, file_cell.y);

  // 1. Open the popup by clicking File.
  input.click(file_cell.x, file_cell.y);
  sleep_ms(400);
  dump(gs, probe, "after click File (popup open)");

  // 2. Hover the first item.
  auto cellpt = item_cell(probe.item_new);
  input.move(cellpt.x, cellpt.y);
  sleep_ms(400);
  dump(gs, probe, "after hover New");

  // 3. Move to the second item; New must be fully erased now.
  cellpt = item_cell(probe.item_open);
  input.move(cellpt.x, cellpt.y);
  sleep_ms(400);
  dump(gs, probe, "after hover Open (New must be clean)");

  // 4. Move to the third item; Open must be fully erased now.
  cellpt = item_cell(probe.item_save);
  input.move(cellpt.x, cellpt.y);
  sleep_ms(400);
  dump(gs, probe, "after hover Save (Open must be clean)");

  // 5. Leave the popup into the content area; Save must be fully erased.
  auto content_pos = convert_point_to_screen(5 * cw, 5 * ch, probe.content);
  auto content_cell = to_cell(content_pos, { 10 * cw, 2 * ch });
  input.move(content_cell.x, content_cell.y);
  sleep_ms(400);
  dump(gs, probe, "after leave to content (Save must be clean)");

  terminal.shutdown();
  loop.join();
  std::printf("probe done\n");
  return 0;
}
