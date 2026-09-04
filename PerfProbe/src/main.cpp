// Benchmarks the text screen's emission path (TextScreen::print_rows_region
// + terminal flush) by counting what actually reaches the terminal: bytes
// written, flush (sync) calls, and CPU time. The console output is replaced
// by a counting sink, so the numbers below are the exact byte stream and
// flush count the terminal would receive -- the two quantities that decide
// how much of the per-repaint latency (tens of ms per ConPTY flush in the
// demo logs) is unavoidable.
//
// Results go to stderr; stdout carries the terminal's own escape stream
// (separate the two when redirecting).
//
// Scenarios mirror the MenuBarDemo interactions:
//   S1  first full paint of the frame (show)
//   S2  full refresh with NOTHING changed           (dup emission?)
//   S3  region repaint of the status panel, unchanged content
//   S4  pointer moved, damage covers the whole panel
//   S5  pointer moved, damage covers only the changed digits
//   S6  popup menu opens   (show_window -> refresh)
//   S7  popup menu closes  (hide_window -> refresh)
//   S8  menu rollover (arm highlight toggle)
//   S9  two UNCHANGED damaged regions in one pass
//   S10 menu and pointer both changed, one pass     (flush count)
//   S11 full refresh after all the churn            (shadow consistency)
//
// S1/S6/S7 are single-shot (show/hide paint once); the rest are averaged
// over N iterations.

#include <tui++/BorderLayout.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Graphics.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>
#include <tui++/terminal/Terminal.h>

#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <streambuf>
#include <string>

using namespace tui;

namespace {

// Counts every byte and every flush (pubsync) the terminal layer writes.
struct CountingSink: std::streambuf {
  size_t bytes = 0;
  size_t syncs = 0;

  std::streamsize xsputn(const char *, std::streamsize n) override {
    this->bytes += size_t(n);
    return n;
  }

  int overflow(int c) override {
    if (c != EOF) {
      ++this->bytes;
    }
    return c;
  }

  int sync() override {
    ++this->syncs;
    return 0;
  }
};

struct Stat {
  double ms = 0;
  size_t bytes = 0;
  size_t syncs = 0;
};

// Runs `fn` once per iteration with the counting sink installed and returns
// the averaged statistics (bytes and syncs are constant across iterations
// when every iteration repaints; for single-shot scenarios pass 1).
Stat measure(int iterations, const std::function<void()> &fn) {
  CountingSink sink;
  auto old = std::cout.rdbuf(&sink);
  auto t0 = std::chrono::steady_clock::now();
  for (auto i = 0; i < iterations; ++i) {
    fn();
  }
  auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  std::cout.rdbuf(old);
  return { ms / iterations, sink.bytes / size_t(iterations), sink.syncs / size_t(iterations) };
}

void report(const char *name, const Stat &s) {
  std::fprintf(stderr, "%-46s %10zu B  %4zu flush  %8.3f ms\n", name, s.bytes, s.syncs, s.ms);
}

// Bottom status panel whose last line changes with the "pointer position",
// standing in for the demo's HoverPanel.
class StatusPanel: public Component {
  std::string pointer_text;

public:
  StatusPanel() {
    set_opaque(true);
    set_background_color(Color { 24, 26, 34 });
    set_foreground_color(Color { 200, 200, 205 });
  }

  void set_pointer(int x, int y) {
    this->pointer_text = "pointer=(x=" + std::to_string(x) + ", y=" + std::to_string(y) + ")";
  }

  void paint(Graphics &g) override {
    auto w = get_width();
    auto h = get_height();
    auto metrics = screen.get_text_metrics();
    auto line = metrics->get_line_height();

    g.set_background_color(Color { 24, 26, 34 });
    g.fill_rect(0, 0, w, h);
    g.set_foreground_color(Color { 200, 200, 205 });
    g.draw_string("hover: (none)", 1, 0);
    g.draw_string("bounds=(x=0, y=0, w=" + std::to_string(w) + ", h=" + std::to_string(h) + ")", 1, line);
    g.draw_string(this->pointer_text.empty() ? "pointer=(x=0, y=0)" : this->pointer_text, 1, 2 * line);
  }
};

} // namespace

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  terminal.set_type("text");

  auto size = terminal.get_size();
  std::fprintf(stderr, "terminal size: %dx%d\n", size.width, size.height);

  auto frame = make_component<Frame>();
  frame->set_size(size);
  // Not shown yet (Window::init makes it invisible): S1 measures the very
  // first set_visible(true).

  auto content = make_component<Panel>();
  content->set_background_color(Color { 0, 0, 128 });
  frame->add(content);

  auto status = make_component<StatusPanel>();
  auto cell = screen.get_text_metrics()->get_line_height();
  status->set_preferred_size(Dimension { 0, 3 * cell });
  status->set_pointer(1, 1);
  frame->add(status, BorderLayout::SOUTH);

  auto menu_bar = make_component<MenuBar>();
  auto file_menu = make_component<Menu>("File");
  auto item = make_component<MenuItem>("Open");
  file_menu->add(item);
  menu_bar->add(file_menu);
  frame->set_menu_bar(menu_bar);

  // Geometry of the interesting parts, filled in once layout has run (see
  // the warm-up below). Captured by reference by the scenario lambdas.
  Rectangle status_bounds { };
  Rectangle file_bounds { };
  Rectangle digits_span { };

  // Alternates between pointer texts of equal shape ("pointer=(x=40, y=1)"
  // vs "pointer=(x=41, y=2)"), so every iteration changes the same few
  // cells -- like a pointer crossing one cell.
  auto tick = 0;
  auto next_pointer = [&] {
    status->set_pointer(tick & 1 ? 41 : 40, (tick >> 1) & 1 ? 2 : 1);
    ++tick;
  };

  auto first_show = [&] {
    frame->set_visible(true);
  };
  auto full_refresh = [&] {
    screen.refresh();
  };
  auto damage_status = [&] {
    screen.add_damage(status_bounds);
    screen.repaint_damaged();
  };
  auto status_region = [&] {
    damage_status();
  };
  auto changed_pointer = [&] {
    next_pointer();
    damage_status();
  };
  auto changed_digits = [&] {
    next_pointer();
    screen.add_damage(digits_span);
    screen.repaint_damaged();
  };
  auto popup_open = [&] {
    file_menu->set_popup_menu_visible(true);
  };
  auto popup_close = [&] {
    file_menu->set_popup_menu_visible(false);
  };
  auto rollover = [&] {
    file_menu->set_armed((tick & 1) != 0);
    file_menu->repaint();
    screen.repaint_damaged();
  };
  auto two_regions = [&] {
    screen.add_damage(file_bounds);
    screen.add_damage(status_bounds);
    screen.repaint_damaged();
  };
  auto both_changed = [&] {
    file_menu->set_armed((tick & 1) != 0);
    file_menu->repaint();
    next_pointer();
    screen.add_damage(file_bounds);
    screen.add_damage(status_bounds);
    screen.repaint_damaged();
  };

  constexpr auto N = 300;

  report("S1 first Window::show (full paint)", measure(1, first_show));

  // Warm up: popup window creation, a few paints, final layout.
  popup_open();
  popup_close();
  full_refresh();
  status_region();
  changed_pointer();

  status_bounds = Rectangle { status->get_location_on_screen(), status->get_size() };
  file_bounds = Rectangle { file_menu->get_location_on_screen(), file_menu->get_size() };
  std::fprintf(stderr, "status panel: (%d, %d %dx%d), file menu: (%d, %d %dx%d)\n", //
              status_bounds.x, status_bounds.y, status_bounds.width, status_bounds.height, //
              file_bounds.x, file_bounds.y, file_bounds.width, file_bounds.height);

  // The pointer line is "pointer=(x=.., y=.)": its digits live in columns
  // 12..18 of the drawn text (screen x = status.x + 13 .. + 20).
  digits_span = Rectangle { status_bounds.x + 12, status_bounds.y + 2 * cell, 9, 1 };

  // Re-show so the measured S2..S9 all start from the same visible state.
  first_show();

  report("S2 full refresh, nothing changed", measure(N, full_refresh));
  report("S3 status region repaint, unchanged", measure(N, status_region));
  report("S4 pointer moved, panel-wide damage", measure(N, changed_pointer));
  report("S5 pointer moved, digits-only damage", measure(N, changed_digits));
  report("S6 popup open (show_window -> refresh)", measure(1, popup_open));
  report("S7 popup close (hide_window -> refresh)", measure(1, popup_close));
  report("S8 menu rollover (arm highlight)", measure(N, rollover));
  report("S9 two UNCHANGED regions, one pass", measure(N, two_regions));
  report("S10 menu + pointer changed, one pass", measure(N, both_changed));
  report("S11 full refresh after churn (shadow consistency)", measure(1, full_refresh));

  // Leave the popup in a known state and close the frame cleanly so the
  // terminal restore sequence runs while stdout is back on the console.
  file_menu->set_popup_menu_visible(false);
  frame->set_visible(false);
  return 0;
}
