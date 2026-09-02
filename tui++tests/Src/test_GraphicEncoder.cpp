#include <tui++/terminal/graphic/GraphicEncoder.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

using namespace tui;

// Round-trips images through GraphicEncoder: the emitted sixel stream is
// decoded back into pixels and compared to the input. Guards the band
// classification, which once kept only two colors per column and dropped (or
// recolored) thin glyph pixels sharing a band with a border color.
namespace {

struct RGB {
  unsigned char r, g, b;
};

struct Decoder {
  int width, height;
  std::vector<RGB> out;
  std::array<RGB, 256> palette { };
  int pen_x = 0;
  int band_row = 0;
  int color = -1;

  Decoder(int w, int h) :
      width(w), height(h), out(std::size_t(w) * h) {
  }

  void apply(int mask, int count) {
    for (auto k = 0; k < count; ++k) {
      auto x = pen_x + k;
      if (x < width and color >= 0) {
        for (auto r = 0; r < 6; ++r) {
          if (mask & (1 << r)) {
            auto y = band_row * 6 + r;
            if (y < height) {
              out[std::size_t(y) * width + x] = palette[std::size_t(color)];
            }
          }
        }
      }
    }
    pen_x += count;
  }

  bool decode(const std::string &s) {
    std::size_t i = 0;
    if (s.compare(0, 2, "\x1bP") != 0) {
      return false;
    }
    i = 2;
    while (i < s.size() and s[i] != 'q') {
      ++i;
    }
    if (i >= s.size()) {
      return false;
    }
    ++i;
    // Raster attributes: " Pan ; Pad ; Pv ; Ph
    if (i >= s.size() or s[i] != '"') {
      return false;
    }
    ++i;
    for (auto field = 0; field < 4; ++field) {
      while (i < s.size() and s[i] >= '0' and s[i] <= '9') {
        ++i;
      }
      if (field < 3 and s[i] == ';') {
        ++i;
      }
    }
    while (i < s.size()) {
      auto c = s[i];
      if (c == '#') {
        ++i;
        auto n = 0;
        while (i < s.size() and s[i] >= '0' and s[i] <= '9') {
          n = n * 10 + (s[i] - '0');
          ++i;
        }
        if (i < s.size() and s[i] == ';') {
          ++i; // #N;2;R;G;B palette definition
          auto rgb = std::array<int, 4> { };
          for (auto f = 0; f < 4; ++f) {
            auto v = 0;
            while (i < s.size() and s[i] >= '0' and s[i] <= '9') {
              v = v * 10 + (s[i] - '0');
              ++i;
            }
            rgb[f] = v;
            if (f < 3 and i < s.size() and s[i] == ';') {
              ++i;
            }
          }
          // K == 2 means RGB; the components are scaled 0..100.
          palette[std::size_t(n)] = RGB { static_cast<unsigned char>(rgb[1] * 255 / 100), static_cast<unsigned char>(rgb[2] * 255 / 100), static_cast<unsigned char>(rgb[3] * 255 / 100) };
        }
        color = n;
      } else if (c == '$') {
        pen_x = 0;
        ++i;
      } else if (c == '-') {
        band_row += 1;
        ++i;
      } else if (c == '!') {
        ++i;
        auto count = 0;
        while (i < s.size() and s[i] >= '0' and s[i] <= '9') {
          count = count * 10 + (s[i] - '0');
          ++i;
        }
        if (i >= s.size()) {
          return false;
        }
        apply(int(s[i] - 0x3F), count);
        ++i;
      } else if (c == '?') {
        apply(0, 1);
        ++i;
      } else {
        apply(int(c - 0x3F), 1);
        ++i;
      }
    }
    return true;
  }
};

bool round_trip(const char *name, const std::vector<RGB> &input, int width, int height) {
  auto packed = std::vector<unsigned char> { };
  packed.reserve(std::size_t(width) * height * 3);
  for (auto const &p : input) {
    packed.push_back(p.r);
    packed.push_back(p.g);
    packed.push_back(p.b);
  }

  auto encoded = GraphicEncoder::encode(packed.data(), width, height, width);
  auto dec = Decoder { width, height };
  if (not dec.decode(encoded)) {
    std::printf("FAIL %s: decode failed\n", name);
    return false;
  }

  auto mismatches = 0;
  for (std::size_t i = 0; i < input.size(); ++i) {
    // The sixel protocol stores colors in a 0..100 range, so 0..255 values do
    // not round-trip exactly; +-2 per channel is expected. Anything bigger is
    // color contamination (the bug being guarded against).
    auto diff = [](unsigned char a, unsigned char b) {
      return int(a) > int(b) ? int(a) - int(b) : int(b) - int(a);
    };
    if (diff(dec.out[i].r, input[i].r) > 2 or diff(dec.out[i].g, input[i].g) > 2 or diff(dec.out[i].b, input[i].b) > 2) {
      ++mismatches;
    }
  }
  if (mismatches == 0) {
    std::printf("PASS %s\n", name);
    return true;
  }
  std::printf("FAIL %s: %d mismatches\n", name, mismatches);
  return false;
}

}

void test_GraphicEncoder() {
  constexpr auto W = 40;

  // The reported bug: a band holding a 3-pixel cyan border, a 2-pixel menu
  // background and a 1-pixel white glyph row in the same columns. The old
  // two-color-per-column classification dropped the glyph pixel.
  {
    constexpr auto H = 6;
    auto img = std::vector<RGB>(std::size_t(W) * H);
    for (auto x = 0; x < W; ++x) {
      for (auto y = 0; y < H; ++y) {
        auto px = RGB { 16, 16, 16 }; // menu background
        if (y < 3) {
          px = RGB { 0, 255, 255 }; // cyan border
        }
        if (y == 5 and x >= 4 and x < 8) {
          px = RGB { 240, 240, 240 }; // glyph top bar
        }
        img[std::size_t(y) * W + x] = px;
      }
    }
    assert(round_trip("border+bg+glyph in one band", img, W, H));
  }

  // Two full cells (32 rows): glyph pixels at different band offsets plus a
  // full-width yellow line, exercising several bands and band boundaries.
  {
    constexpr auto H = 32;
    auto img = std::vector<RGB>(std::size_t(W) * H);
    for (auto x = 0; x < W; ++x) {
      for (auto y = 0; y < H; ++y) {
        auto px = RGB { 32, 32, 32 }; // background
        if (y < 2) {
          px = RGB { 0, 255, 255 }; // top border
        }
        if ((y == 5 or y == 8 or y == 12 or y == 30) and x >= 4 and x < 8) {
          px = RGB { 240, 240, 240 }; // glyphs at band offsets
        }
        if (y == 20) {
          px = RGB { 255, 255, 0 }; // yellow hline
        }
        img[std::size_t(y) * W + x] = px;
      }
    }
    assert(round_trip("multi-band cells", img, W, H));
  }

  // Four distinct colors sharing columns in one band.
  {
    constexpr auto H = 6;
    auto img = std::vector<RGB>(std::size_t(W) * H);
    for (auto x = 0; x < W; ++x) {
      for (auto y = 0; y < H; ++y) {
        auto px = RGB { 24, 24, 24 };
        if (y == 0) {
          px = RGB { 0, 255, 255 }; // cyan
        }
        if (y == 1) {
          px = RGB { 255, 255, 0 }; // yellow
        }
        if (y == 2) {
          px = RGB { 0, 0, 255 }; // blue
        }
        if (y == 3 and x % 2 == 0) {
          px = RGB { 240, 240, 240 }; // white dots
        }
        if (y == 5 and x % 3 == 0) {
          px = RGB { 0, 255, 0 }; // green dots
        }
        img[std::size_t(y) * W + x] = px;
      }
    }
    assert(round_trip("four colors per column", img, W, H));
  }

  // Every palette index must be defined exactly once per image: terminals
  // keep a single palette per image, so redefining an entry in a later band
  // recolor the earlier bands that used it (the reported "colors change
  // chaotically while the window draws").
  {
    constexpr auto H = 12;
    auto img = std::vector<RGB>(std::size_t(W) * H);
    for (auto x = 0; x < W; ++x) {
      for (auto y = 0; y < H; ++y) {
        auto px = RGB { 16, 16, 16 };
        if (y < 6) {
          px = RGB { 255, 255, 255 }; // first band: white
        } else {
          px = RGB { 255, 255, 0 }; // second band: yellow
        }
        if (y == 10) {
          px = RGB { 0, 0, 255 }; // blue over the second band's fill
        }
        img[std::size_t(y) * W + x] = px;
      }
    }

    auto packed = std::vector<unsigned char> { };
    packed.reserve(std::size_t(W) * H * 3);
    for (auto const &p : img) {
      packed.push_back(p.r);
      packed.push_back(p.g);
      packed.push_back(p.b);
    }
    auto encoded = GraphicEncoder::encode(packed.data(), W, H, W);

    auto definitions = std::vector<int> { };
    for (std::size_t i = 0; i < encoded.size();) {
      if (encoded[i] == '#') {
        auto n = 0;
        auto j = i + 1;
        while (j < encoded.size() and encoded[j] >= '0' and encoded[j] <= '9') {
          n = n * 10 + (encoded[j] - '0');
          ++j;
        }
        if (j < encoded.size() and encoded[j] == ';') {
          definitions.push_back(n); // "#N;..." definition (not just a color select)
        }
        i = j;
      } else {
        ++i;
      }
    }
    auto sorted = definitions;
    std::sort(sorted.begin(), sorted.end());
    auto duplicates = std::adjacent_find(sorted.begin(), sorted.end());
    assert(duplicates == sorted.end());

    assert(round_trip("one palette for the whole image", img, W, H));
  }
}
