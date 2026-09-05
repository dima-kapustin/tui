// Resize reproduction probe for the text screen.
//
// Mirrors MenuBarDemo (frame + menu bar + bordered content panel), runs the
// real text-screen event loop in a thread, optionally opens a popup menu,
// then resizes the process console and feeds the resize through the real
// path (screen.resized(), posted on the event queue so it runs on the
// dispatch thread exactly as Terminal::new_resize_event would).
//
// stdout is redirected to resize_probe_stream.bin (the raw escape stream);
// diagnostics go to stderr. tools/text_render.py reconstructs the final
// screen from the captured stream.
//
// Build (from the repo root):
//   g++ -g -std=gnu++26 -DUNICODE -Itui++/Inc tools/resize_probe.cpp \
//       build-dbg/tui++/libtui++.a -lkernel32 -luser32 -lgdi32 -o build-dbg/resize_probe.exe
//
// Run:
//   build-dbg/resize_probe.exe [popup] 2> resize_trace.log

#include <tui++/BorderLayout.h>
#include <tui++/Component.h>
#include <tui++/Event.h>
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/Screen.h>
#include <tui++/border/EmptyBorder.h>
#include <tui++/border/LineBorder.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/TextMetrics.h>
#include <tui++/util/log.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string_view>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using namespace tui;

// The console output handle, duplicated before stdout is redirected: freopen
// closes the CRT's stdout handle, which would otherwise take the console
// with it.
HANDLE console_out = nullptr;

namespace {

void sleep_ms(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Resizes the process console window to w x h cells. The window may only
// shrink into the buffer it has and only grow into a buffer large enough
// for it, so the window is shrunk first, then the buffer grown, then the
// window set to the target.
bool resize_console(int w, int h) {
  auto hOut = console_out;
  CONSOLE_SCREEN_BUFFER_INFO csbi { };
  if (not ::GetConsoleScreenBufferInfo(hOut, &csbi)) {
    std::fprintf(stderr, "GetConsoleScreenBufferInfo failed: %lu\n", ::GetLastError());
    return false;
  }
  auto cw = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  auto ch = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  std::fprintf(stderr, "console was %dx%d (buffer %dx%d)\n", cw, ch, csbi.dwSize.X, csbi.dwSize.Y);

  if (w < cw or h < ch) {
    SMALL_RECT r { 0, 0, SHORT(std::min(w, cw) - 1), SHORT(std::min(h, ch) - 1) };
    if (not ::SetConsoleWindowInfo(hOut, TRUE, &r)) {
      std::fprintf(stderr, "SetConsoleWindowInfo(shrink) failed: %lu\n", ::GetLastError());
      return false;
    }
  }

  COORD size { SHORT(std::max(csbi.dwSize.X, SHORT(w))), SHORT(std::max(csbi.dwSize.Y, SHORT(h))) };
  if (not ::SetConsoleScreenBufferSize(hOut, size)) {
    std::fprintf(stderr, "SetConsoleScreenBufferSize(%dx%d) failed: %lu\n", size.X, size.Y, ::GetLastError());
    return false;
  }

  SMALL_RECT rect { 0, 0, SHORT(w - 1), SHORT(h - 1) };
  if (not ::SetConsoleWindowInfo(hOut, TRUE, &rect)) {
    std::fprintf(stderr, "SetConsoleWindowInfo(%dx%d) failed: %lu\n", w, h, ::GetLastError());
    return false;
  }
  return true;
}

// The console window updates apply asynchronously (the hidden console under
// the test harness lags SetConsoleWindowInfo); poll until the reported size
// matches the target.
bool wait_for_size(int w, int h) {
  for (auto n = 0; n < 200; ++n) {
    CONSOLE_SCREEN_BUFFER_INFO csbi { };
    ::GetConsoleScreenBufferInfo(console_out, &csbi);
    auto cw = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    auto ch = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    if (cw == w and ch == h) {
      return true;
    }
    sleep_ms(10);
  }
  return false;
}

// Feeds mouse events into the screen the way Terminal's input path does.
struct Input {
  MouseEvent::Modifiers modifiers { };

  void post(std::shared_ptr<Event> const &event) {
    screen.post(event);
  }

  void press(MousePressEvent::Type type, MousePressEvent::Button button, int x, int y) {
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
    auto p = screen.convert_mouse_point(x, y);
    auto window = screen.get_window_at(p);
    if (window) {
      p = convert_point_from_screen(p, window);
    }
    post(std::make_shared<MouseClickEvent>(window, MousePressEvent::LEFT_BUTTON, modifiers, p.x, p.y, 1, false));
  }
};

} // namespace

int main(int argc, char *argv[]) {
  auto with_popup = argc > 1 and std::string_view { argv[1] } == "popup";
  std::fprintf(stderr, "resize probe: popup=%d\n", int(with_popup));

  auto stdout_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
  ::DuplicateHandle(::GetCurrentProcess(), stdout_handle, ::GetCurrentProcess(), &console_out, 0, FALSE, DUPLICATE_SAME_ACCESS);

  // The screen is sized from the terminal when it is created; give the
  // console a known starting size first (and let the window update land).
  resize_console(80, 24);
  wait_for_size(80, 24);

  freopen("resize_probe_stream.bin", "wb", stdout);
  terminal.set_type("text");
  util::event_log = &std::cerr;

  std::fprintf(stderr, "after set_type: screen %dx%d\n", screen.get_size().width, screen.get_size().height);

  auto cell = screen.get_text_metrics()->get_line_height();

  // MenuBarDemo's tree, on the cobalt/ice palette (cyan border).
  auto frame = make_component<Frame>();
  frame->set_background_color(GREEN_COLOR);
  frame->set_size(screen.get_size());
  frame->set_name("main frame");
  frame->get_content_pane()->set_border(std::make_shared<EmptyBorder>(2 * cell, 2 * cell, 2 * cell, 2 * cell));

  auto content = make_component<Panel>();
  content->set_name("content");
  frame->add(content);
  content->set_background_color(Color { 0x02, 0x3E, 0x8A });
  content->set_border(std::make_shared<LineBorder>(Stroke::HEAVY, Color { 0x90, 0xE0, 0xEF }));

  auto file_menu = make_component<Menu>("File");
  file_menu->add(make_component<MenuItem>("New"));
  file_menu->add(make_component<MenuItem>("Open"));
  file_menu->add(make_component<MenuItem>("Save"));
  file_menu->add_separator();
  file_menu->add(make_component<MenuItem>("Exit"));

  auto edit_menu = make_component<Menu>("Edit");
  edit_menu->add(make_component<MenuItem>("Cut"));

  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(file_menu);
  menu_bar->add(edit_menu);
  frame->set_menu_bar(menu_bar);

  // The demo's click-to-toggle on the top-level menu.
  file_menu->add_listener([file_menu](MousePressEvent &e) {
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

  sleep_ms(400); // initial full paint

  auto input = Input { };
  if (with_popup) {
    // Open the popup directly (as the tests do) and arm its first item so
    // the stretched selection highlight is visible after the resize.
    file_menu->set_popup_menu_visible(true);
    auto popup = file_menu->get_popup_menu();
    if (not popup->get_components().empty()) {
      if (auto item = std::dynamic_pointer_cast<MenuItem>(popup->get_components()[0])) {
        item->set_armed(true);
      }
    }
    sleep_ms(400);
    std::fprintf(stderr, "popup open: %d\n", int(file_menu->is_popup_menu_visible()));
    if (auto pwin = file_menu->get_popup_menu()->get_containing_window()) {
      auto sz = pwin->get_size();
      std::fprintf(stderr, "popup window before resize: %dx%d\n", sz.width, sz.height);
    }
  }

  // Resize the console and run the same resize handling the terminal's
  // WINDOW_BUFFER_SIZE_EVENT would (posted so it runs on the dispatch
  // thread, ordered with the events around it).
  resize_console(100, 30);
  wait_for_size(100, 30);
  std::fprintf(stderr, "after console resize: terminal %dx%d\n", terminal.get_size().width, terminal.get_size().height);
  screen.post([] {
    screen.resized();
  });
  sleep_ms(400);
  std::fprintf(stderr, "after resized: screen %dx%d\n", screen.get_size().width, screen.get_size().height);

  auto frame_size = frame->get_size();
  std::fprintf(stderr, "after resize: frame %dx%d\n", frame_size.width, frame_size.height);
  if (with_popup) {
    if (auto pwin = file_menu->get_popup_menu()->get_containing_window()) {
      auto sz = pwin->get_size();
      auto loc = pwin->get_location();
      std::fprintf(stderr, "after resize: popup window %dx%d at (%d, %d)\n", sz.width, sz.height, loc.x, loc.y);
    }
  }

  sleep_ms(200);
  terminal.shutdown();
  loop.join();
  std::fprintf(stderr, "probe done\n");
  return 0;
}
