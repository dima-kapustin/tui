// Probe: the pixel (sixel) backend's drop shadows.
//
// The automated suite verifies the text screen's shadows cell by cell (see
// test_Shadow). This probe verifies the same scene on the pixel screen, where
// the shadow is a true alpha blend of the framebuffer's pixels: the pixels
// under the popup keep the popup's colors, the rim the popup does not cover
// is the frame's color shifted towards the shadow's, and the global switch
// removes it again.
//
// Build (from the repo root):
//   g++ -g -std=gnu++26 -DUNICODE -Itui++/Inc tools/probe_shadow.cpp \
//       build/tui++/libtui++.a -lkernel32 -luser32 -lgdi32 -lwinspool \
//       -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 \
//       -o tools/probe_shadow.exe
//
// Run (stdout is redirected by the probe itself; the terminal needs sixel
// support and the query may be skipped with the library's own escape hatch):
//   TUI_FORCE_SIXEL=1 tools/probe_shadow.exe 2> shadow_probe.log

#include <tui++/BorderLayout.h>
#include <tui++/Frame.h>
#include <tui++/Panel.h>
#include <tui++/PopupMenu.h>
#include <tui++/Screen.h>
#include <tui++/Shadow.h>

#include <tui++/event/InvocationEvent.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/sixel/SixelScreen.h>

#include <cmath>
#include <cstdio>
#include <memory>

using namespace tui;

namespace {

constexpr auto FRAME_COLOR = Color { 0, 200, 0 };
constexpr auto POPUP_COLOR = Color { 0, 0, 200 };

int failures = 0;

void check(bool ok, char const *what) {
  std::fprintf(stderr, "%s %s\n", ok ? "PASS" : "FAIL", what);
  if (not ok) {
    ++failures;
  }
}

Color shade(Color const &base, Shadow const &shadow) {
  auto blend = [&](uint8_t from, uint8_t to) {
    return uint8_t(std::lround(from * (1 - shadow.opacity) + to * shadow.opacity));
  };
  return Color { blend(base.red(), shadow.color.red()), blend(base.green(), shadow.color.green()), blend(base.blue(), shadow.color.blue()) };
}

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
}

}

int main() {
  std::setvbuf(stderr, nullptr, _IONBF, 0);

  Terminal::Singleton singleton;
  terminal.set_type("sixel");
  auto &gs = static_cast<SixelScreen &>(screen);
  auto width = gs.get_pixel_width();
  auto height = gs.get_pixel_height();
  std::fprintf(stderr, "probe: %dx%d px, cell %dx%d\n", width, height, gs.get_cell_width(), gs.get_cell_height());

  auto pixel = [&](int x, int y) {
    auto *px = gs.debug_pixels() + (std::size_t(y) * width + x) * 3;
    return Color { px[0], px[1], px[2] };
  };

  auto frame = make_component<Frame>();
  frame->set_size({ width, height });
  frame->set_background_color(FRAME_COLOR);

  auto popup_menu = make_component<PopupMenu>();
  popup_menu->set_background_color(POPUP_COLOR);
  popup_menu->add("Alpha");
  popup_menu->add("Beta");
  for (auto i = 0; i < popup_menu->get_component_count(); ++i) {
    popup_menu->Component::get_component(i)->set_background_color(POPUP_COLOR);
  }

  frame->set_visible(true);
  drain_events();
  popup_menu->show(frame, 40, 40);
  drain_events();
  screen.refresh();

  auto popup_window = popup_menu->get_containing_window();
  if (not popup_window) {
    std::fprintf(stderr, "FAIL no popup window\n");
    return 1;
  }
  auto popup = popup_window->get_bounds();
  std::fprintf(stderr, "probe: popup at (%d,%d %dx%d)\n", popup.x, popup.y, popup.width, popup.height);

  auto themed = laf::LookAndFeel::get<std::optional<Shadow>>("PopupMenu.Shadow");
  check(themed.has_value(), "the theme defines a popup shadow");
  auto shadowed = shade(FRAME_COLOR, *themed);

  check(pixel(popup.x + popup.width / 2, popup.y + popup.height / 2) == POPUP_COLOR, "the popup's face keeps its own color");
  check(pixel(popup.right() + themed->offset.x / 2, popup.y + popup.height / 2) == shadowed, "the right rim is the frame's color shaded");
  check(pixel(popup.x + themed->offset.x, popup.bottom() + themed->offset.y / 2) == shadowed, "the bottom rim is the frame's color shaded");
  check(pixel(popup.x - 4 * gs.get_cell_width(), popup.y) == FRAME_COLOR, "the frame away from the popup keeps its color");

  Shadow::set_enabled(false);
  screen.refresh();
  check(pixel(popup.right() + themed->offset.x / 2, popup.y + popup.height / 2) == FRAME_COLOR, "the global switch removes the shadow");
  Shadow::set_enabled(true);
  screen.refresh();
  check(pixel(popup.right() + themed->offset.x / 2, popup.y + popup.height / 2) == shadowed, "switching it back paints it again");

  // A coarse picture of the scene: one character per 8x8 pixel block -- '.'
  // the frame, '#' the popup's face, ':' the shadow, '?' anything else (the
  // popup's border, the menu text).
  std::fprintf(stderr, "probe: the scene around the popup (one char per 8x8 px)\n");
  for (auto y = ((popup.y - 16) / 8) * 8; y < popup.bottom() + 24; y += 8) {
    auto row = std::string { };
    for (auto x = ((popup.x - 16) / 8) * 8; x < popup.right() + 24; x += 8) {
      auto color = pixel(std::clamp(x, 0, width - 1), std::clamp(y, 0, height - 1));
      row += color == FRAME_COLOR ? '.' : color == POPUP_COLOR ? '#' : color == shadowed ? ':' : '?';
    }
    std::fprintf(stderr, "  %s\n", row.c_str());
  }

  std::fprintf(stderr, failures ? "probe: %d failure(s)\n" : "probe: ok\n", failures);
  return failures ? 1 : 0;
}
