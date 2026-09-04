// Encodes a raw RGB file (w h from argv, row-major 3-byte pixels) into a
// sixel DCS on stdout, so encoder output can be diffed across builds.
#include <tui++/terminal/sixel/SixelEncoder.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::fprintf(stderr, "usage: %s width height pixels.rgb\n", argv[0]);
    return 2;
  }
  auto w = std::atoi(argv[1]);
  auto h = std::atoi(argv[2]);
  auto f = std::fopen(argv[3], "rb");
  if (not f) {
    return 2;
  }
  auto rgb = std::vector<uint8_t>(std::size_t(w) * h * 3);
  auto n = std::fread(rgb.data(), 1, rgb.size(), f);
  std::fclose(f);
  if (n != rgb.size()) {
    return 2;
  }
  auto out = tui::SixelEncoder::encode(rgb.data(), w, h, w);
  std::fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
