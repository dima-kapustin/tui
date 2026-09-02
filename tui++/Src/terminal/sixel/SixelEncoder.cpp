#include <tui++/terminal/sixel/SixelEncoder.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace tui {

namespace {

constexpr int BAND_HEIGHT = 6;        // one sixel band covers 6 pixel rows
constexpr int MAX_PALETTE_SIZE = 256; // palette entries per image

struct RGB {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

void append_int(std::string &out, unsigned value) {
  char buf[16];
  auto result = std::to_chars(buf, buf + std::size(buf), value);
  out.append(buf, result.ptr);
}

void append_raster_attributes(std::string &out, int width, int height) {
  // DECGRA: `" Pan ; Pad ; Ph ; Pv ` -- Pan/Pad are the pixel aspect ratio
  // (1:1, square pixels), Ph is the horizontal extent (width) and Pv the
  // vertical extent (height), both in pixels. xterm (graphics_sixel.c),
  // Windows Terminal (SixelParser.cpp) and the DEC VT330/VT340 manuals all
  // agree on this order; emitting them swapped made xterm treat every wide
  // image as if it were that many pixels tall, scrolling the window as the
  // image was "written past the bottom line".
  out += "\"1;1;";
  append_int(out, width);
  out += ';';
  append_int(out, height);
}

void append_palette(std::string &out, std::vector<RGB> const &palette) {
  for (auto i = 0; i < int(palette.size()); ++i) {
    // A sixel palette entry is `#Pc;Pu;Px;Py;Pz`: palette index, color
    // coordinate system (2 = RGB), then the three components in the 0..100
    // range. The RGB values are stored here as 0..255 and scaled down.
    out += '#';
    append_int(out, i);
    out += ";2;";
    append_int(out, palette[i].red * 100 / 255);
    out += ';';
    append_int(out, palette[i].green * 100 / 255);
    out += ';';
    append_int(out, palette[i].blue * 100 / 255);
  }
}

// Emits `count` repetitions of the data char `c`, using the `!N` run-length
// form for runs long enough to make it smaller than the raw chars.
void append_run(std::string &out, int count, char c) {
  if (count <= 3) {
    out.append(std::size_t(count), c);
  } else {
    out += '!';
    append_int(out, count);
    out += c;
  }
}

// Keeps the `bits` most significant bits of an 8-bit channel.
uint8_t quantize_channel(uint8_t value, int bits) {
  return value & uint8_t(0xFF << (8 - bits));
}

uint32_t quantize(const uint8_t *rgb, int bits) {
  return (uint32_t(quantize_channel(rgb[0], bits)) << 16) | //
      (uint32_t(quantize_channel(rgb[1], bits)) << 8) | //
      quantize_channel(rgb[2], bits);
}

// Builds one palette for the whole image. The palette is emitted once, before
// the first band: terminals keep a single palette per image, so redefining an
// entry in a later band would recolor every pixel of the earlier bands that
// shares the index (the per-band palettes made parts of the image flash
// through random colors while it drew).
//
// Exact colors are kept while there are at most MAX_PALETTE_SIZE of them
// (the editor's handful of colors round-trips exactly); otherwise the color
// resolution is reduced until they fit. `bits` is the channel resolution used
// to classify pixels (8 == exact colors).
void build_palette(const uint8_t *rgb, int width, int height, int stride, std::vector<RGB> &palette, std::unordered_map<uint32_t, int> &index_map, int &bits) {
  // Pass 1: the distinct exact colors of the image.
  auto exact = std::vector<RGB> { };
  auto exact_seen = std::unordered_map<uint32_t, int> { };
  for (auto y = 0; y < height; ++y) {
    for (auto x = 0; x < width; ++x) {
      auto const *px = rgb + (y * stride + x) * 3;
      auto key = quantize(px, 8);
      if (exact_seen.find(key) == exact_seen.end()) {
        exact_seen.emplace(key, int(exact.size()));
        exact.push_back({ px[0], px[1], px[2] });
      }
    }
  }

  if (int(exact.size()) <= MAX_PALETTE_SIZE) {
    palette = std::move(exact);
    index_map = std::move(exact_seen);
    bits = 8;
    return;
  }

  // Pass 2: quantize the exact colors until they fit one palette. Two bits
  // per channel yields at most 64 colors, so the loop always terminates.
  for (bits = 5; bits >= 2; --bits) {
    palette.clear();
    index_map.clear();
    for (auto const &color : exact) {
      auto px = std::array<uint8_t, 3> { color.red, color.green, color.blue };
      auto key = quantize(px.data(), bits);
      if (index_map.find(key) == index_map.end()) {
        index_map.emplace(key, int(palette.size()));
        palette.push_back(color);
      }
    }
    if (int(palette.size()) <= MAX_PALETTE_SIZE) {
      return;
    }
  }
}

// Classifies the columns of one band: the most frequent color of a column
// becomes its opaque fill, and every other color's rows are recorded as a
// row mask (one mask per palette color). A sixel data char carries a single
// color, so the band is painted with one pass per color: the fill first,
// then each additional color rewound with `$` and drawn over its own rows.
// Every pixel therefore keeps its exact color, however many colors share a
// column (a 1px glyph line over a background that also holds a border keeps
// all three).
// Records, for every palette color, the row mask per column: the rows where
// the pixel has that color. There is no separate opaque fill: every pixel is
// painted exactly once, by its own color's pass, so a renderer that
// mishandles the fill-then-repaint passes cannot bleed one color into rows
// it does not cover (the old fill painted the gap rows between the editor's
// grid cells with the cell color and depended on a later pass repainting
// them, which Windows Terminal sometimes skipped, leaving a light line).
// `pass_order` lists the palette indices from the most to the least frequent
// color, which compresses best.
void classify_band(const uint8_t *rgb, int width, int band_y, int band_height, int stride, int bits, int palette_size, std::unordered_map<uint32_t, int> const &index_map, std::vector<std::vector<uint8_t>> &color_mask, std::vector<int> &pass_order) {
  auto counts = std::array<int, MAX_PALETTE_SIZE> { };

  color_mask.assign(std::size_t(palette_size), std::vector<uint8_t>(std::size_t(width), 0));
  pass_order.resize(std::size_t(palette_size));

  for (auto x = 0; x < width; ++x) {
    for (auto row = 0; row < band_height; ++row) {
      auto const *px = rgb + ((band_y + row) * stride + x) * 3;

      auto idx = 0;
      if (auto pos = index_map.find(quantize(px, bits)); pos != index_map.end()) {
        idx = pos->second;
      }

      color_mask[std::size_t(idx)][std::size_t(x)] |= uint8_t(1 << row);
      ++counts[idx];
    }
  }

  for (auto color = 0; color < palette_size; ++color) {
    pass_order[std::size_t(color)] = color;
  }
  std::sort(pass_order.begin(), pass_order.end(), [&counts](int a, int b) {
    return counts[a] != counts[b] ? counts[a] > counts[b] : a < b;
  });
}

// Emits one band: the band advance, then one pass per color, each pass
// painting only the rows where that color occurs (rewound with `$` between
// the passes). Every pixel is painted exactly once, by its own color, and
// every pass emits exactly one data char per image column, so nothing shifts
// to the right. The image palette is defined once by the caller.
void emit_band(std::string &out, const uint8_t *rgb, int width, int band_y, int band_height, int stride, int bits, int palette_size, std::unordered_map<uint32_t, int> const &index_map, std::vector<std::vector<uint8_t>> &color_mask, std::vector<int> &pass_order) {
  classify_band(rgb, width, band_y, band_height, stride, bits, palette_size, index_map, color_mask, pass_order);

  if (band_y > 0) {
    // Carriage return, then advance to the next band row.
    out += "$-";
  }

  auto first = true;
  for (auto color : pass_order) {
    auto const &mask = color_mask[std::size_t(color)];
    auto any = false;
    for (auto x = 0; x < width; ++x) {
      if (mask[x]) {
        any = true;
        break;
      }
    }
    if (not any) {
      continue;
    }

    if (not first) {
      out += '$';
    }
    first = false;
    out += '#';
    append_int(out, color);
    for (auto x = 0; x < width;) {
      auto m = mask[x];
      auto start = x;
      if (m == 0) {
        while (x < width and mask[x] == 0) {
          ++x;
        }
        append_run(out, x - start, '?');
      } else {
        while (x < width and mask[x] == m) {
          ++x;
        }
        append_run(out, x - start, char(0x3F + m));
      }
    }
  }
}

}

std::string SixelEncoder::encode(const uint8_t *rgb, int width, int height, int stride) {
  if (not rgb or width <= 0 or height <= 0) {
    return { };
  }

  std::string out;
  out.reserve(std::size_t(width) * height / 4);

  auto palette = std::vector<RGB> { };
  auto index_map = std::unordered_map<uint32_t, int> { };
  auto bits = 0;
  build_palette(rgb, width, height, stride, palette, index_map, bits);

  // The image is emitted with a transparent background (P2=1). Its palette is
  // defined once, before the first band: every band then references those
  // entries, so no color is ever redefined mid-image. Every band is painted
  // with one pass per color (most frequent first), each pass drawing only the
  // rows where that color occurs, rewound with a `$` carriage return between
  // the passes. A pixel is painted exactly once, by its own color, so nothing
  // depends on a later pass repainting an earlier one, and every pass
  // advances exactly one column per image column.

  out += "\x1bP0;1;0q";
  append_raster_attributes(out, width, height);
  append_palette(out, palette);

  auto color_mask = std::vector<std::vector<uint8_t>> { };
  auto pass_order = std::vector<int> { };
  for (auto band_y = 0; band_y < height; band_y += BAND_HEIGHT) {
    auto band_height = std::min(BAND_HEIGHT, height - band_y);
    emit_band(out, rgb, width, band_y, band_height, stride, bits, int(palette.size()), index_map, color_mask, pass_order);
  }

  // ST: end of the DCS.
  out += "\x1b\\";
  return out;
}

}
