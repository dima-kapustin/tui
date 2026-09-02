// Benchmarks the sixel encoder on synthetic font-editor-like frames.
// Build: g++ -O3 -std=gnu++26 -I../tui++/Inc bench_encode.cpp ../build/tui++/CMakeFiles/tui++.dir/Src/terminal/sixel/SixelEncoder.cpp.obj -o bench_encode
#include <tui++/terminal/sixel/SixelEncoder.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

static void fill_rect(std::vector<uint8_t> &img, int stride, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
  for (auto yy = y; yy < y + h; ++yy) {
    auto *row = img.data() + (yy * stride + x) * 3;
    for (auto xx = x; xx < x + w; ++xx) {
      *row++ = r;
      *row++ = g;
      *row++ = b;
    }
  }
}

// Random 16x32 glyphs, 95 of them, laid out like the editor strip.
static void draw_glyphs(std::vector<uint8_t> &img, int stride, int per_row, int x0, int y0) {
  for (auto code = 0; code < 95; ++code) {
    auto gx = x0 + (code % per_row) * 16;
    auto gy = y0 + (code / per_row) * 32;
    uint32_t seed = 0x9E3779B9u * (code + 1);
    for (auto yy = 0; yy < 32; ++yy) {
      for (auto xx = 0; xx < 16; ++xx) {
        seed = seed * 1664525u + 1013904223u;
        if ((seed >> 31) & 1) {
          auto *p = img.data() + ((gy + yy) * stride + (gx + xx)) * 3;
          p[0] = 120; p[1] = 125; p[2] = 135;
        }
      }
    }
  }
}

static void draw_grid(std::vector<uint8_t> &img, int stride, int x0, int y0, int cols, int rows, int cell) {
  for (auto ry = 0; ry < rows; ++ry) {
    for (auto rx = 0; rx < cols; ++rx) {
      auto x = x0 + rx * cell, y = y0 + ry * cell;
      auto lit = ((rx * 7 + ry * 13) % 3) != 0;
      fill_rect(img, stride, x, y, cell, cell, lit ? 225 : (ry > 22 ? 40 : 34), lit ? 225 : 40, lit ? 230 : (ry > 22 ? 50 : 42));
      fill_rect(img, stride, x, y + cell - 1, cell, 1, 18, 18, 24);
      fill_rect(img, stride, x + cell - 1, y, 1, cell, 18, 18, 24);
    }
  }
}

int main(int argc, char **argv) {
  // Terminal cells: width x height (chars). Cell pixel size: cw x ch.
  auto cells_w = argc > 1 ? std::atoi(argv[1]) : 120;
  auto cells_h = argc > 2 ? std::atoi(argv[2]) : 30;
  auto cw = argc > 3 ? std::atoi(argv[3]) : 10;
  auto ch = argc > 4 ? std::atoi(argv[4]) : 20;

  auto width = cells_w * cw;
  auto height = cells_h * ch;
  std::vector<uint8_t> img(std::size_t(width) * height * 3, 0);

  // The grid is 16x32 cells of 20px: taller than short terminals; clamp to
  // the frame so the synthetic frame matches what the editor actually paints.
  fill_rect(img, width, 0, 0, width, height, 10, 10, 14);
  fill_rect(img, width, 0, 0, width, 3 * ch / 2, 24, 26, 34); // menu-ish band
  auto grid_cell = 20;
  auto gx0 = 60, gy0 = 140;
  auto grid_rows = std::max(0, (height - gy0) / grid_cell);
  draw_grid(img, width, gx0, gy0, 16, std::min(32, grid_rows), grid_cell);
  auto strip_y = gy0 + 32 * grid_cell + 16;
  auto per_row = std::max(1, (width - 16) / 16);
  auto strip_rows = (95 + per_row - 1) / per_row;
  auto strip_h = 32 * strip_rows;
  auto strip_top = strip_y + strip_h <= height ? strip_y : std::max(0, height - strip_h);
  draw_glyphs(img, width, per_row, 8, strip_top);

  auto t0 = Clock::now();
  auto data = tui::SixelEncoder::encode(img.data(), width, height, width);
  auto t1 = Clock::now();
  auto ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  std::printf("frame %dx%d px (%dx%d cells, cell %dx%d): sixel %zu bytes in %.1f ms\n", //
      width, height, cells_w, cells_h, cw, ch, data.size(), ms);
  return 0;
}
