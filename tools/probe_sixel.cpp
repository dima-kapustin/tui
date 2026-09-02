// Probe for Windows Terminal sixel behaviour, to decide how the font editor
// should implement scrolling and how fast frames can be pushed.
//
// Build (from the repo root):
//   c++ -O3 -std=gnu++26 -DUNICODE -Itui++/Inc tools/probe_sixel.cpp build/tui++/libtui++.a -o tools/probe_sixel.exe
//
// Run inside Windows Terminal (not redirected):  tools/probe_sixel.exe
//
// Phase 1: draws a sixel image with distinct horizontal bands, then scrolls
// the text buffer up 5 rows with CSI S. WATCH: does the image move up with
// the text, stay put, or get corrupted? (Determines whether scrolling can
// reuse the terminal's own buffer scroll instead of re-encoding the frame.)
//
// Phase 2: re-encodes a full-frame sixel image 30 times, measuring how long
// the app-side write takes per frame. The write blocks while the terminal's
// input pipe is full, so a large write time means the terminal is the
// bottleneck. Compare the on-screen update rate with the printed numbers.
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/sixel/SixelScreen.h>

#include <chrono>
#include <cstdio>
#include <thread>

using namespace tui;
using namespace std::string_view_literals;

static void bands(int phase, int width, int height, int cells_w, int cells_h, int cw, int ch) {
  auto &gs = static_cast<SixelScreen &>(screen);
  gs.clear();
  for (auto y = 0; y < cells_h; ++y) {
    for (auto x = 0; x < cells_w; ++x) {
      auto r = uint8_t((x * 7 + y * 3 + phase * 40) % 255);
      auto g = uint8_t((x * 3 + y * 11 + phase * 20) % 255);
      auto b = uint8_t((x * 13 + y * 5 + phase * 10) % 255);
      // Horizontal bands separated by black rows, so any vertical movement
      // is immediately visible.
      if ((y + phase) % 4 == 3) {
        r = g = b = 0;
      }
      gs.fill_pixels({ x * cw, y * ch, cw, ch }, Color { r, g, b });
    }
  }
  gs.fill_pixels({ width / 2 - cw, height - ch, cw, ch }, Color { 255, 255, 255 });
}

int main() {
  Terminal::Singleton singleton;
  terminal.set_type("sixel");
  auto &gs = static_cast<SixelScreen &>(screen);
  auto cw = gs.get_cell_width();
  auto ch = gs.get_cell_height();
  auto width = gs.get_pixel_width();
  auto height = gs.get_pixel_height();
  auto cells_w = width / cw;
  auto cells_h = height / ch;

  std::printf("probe: %dx%d px, %dx%d cells, cell %dx%d\n", width, height, cells_w, cells_h, cw, ch);

  // ---- phase 1: does a sixel image scroll with the text buffer? ----------
  bands(0, width, height, cells_w, cells_h, cw, ch);
  gs.flush();
  std::printf("phase 1: image drawn; scrolling the buffer up 5 rows in 3 s -- watch the image\n");
  std::this_thread::sleep_for(std::chrono::seconds(3));

  terminal << "\x1b[5S"sv;
  terminal.flush();
  std::printf("phase 1: CSI 5S sent. If the image did NOT move with the text, sixel\n"
              "         images are not affected by buffer scrolls in this terminal.\n");
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // ---- phase 2: full-frame re-encode rate --------------------------------
  std::printf("phase 2: 30 full-frame sixel repaints, one every ~30 ms\n");
  auto total_ms = 0.0;
  for (auto i = 0; i < 30; ++i) {
    auto t0 = std::chrono::steady_clock::now();
    bands(i, width, height, cells_w, cells_h, cw, ch);
    gs.flush();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    total_ms += ms;
    std::printf("  frame %2d: app-side %.1f ms (encode+write)\n", i, ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
  std::printf("probe: average app-side %.1f ms/frame; the on-screen rate is whatever\n"
              "       the terminal managed to actually display of those 30 frames.\n", total_ms / 30);

  return 0;
}
