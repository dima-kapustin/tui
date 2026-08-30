#include <tui++/sixel/SixelEncoder.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace tui {

namespace {

constexpr int BAND_HEIGHT = 6;        // one sixel band covers 6 pixel rows
constexpr int MAX_PALETTE_SIZE = 256; // palette entries per band
constexpr int MAX_BAND_COLORS = 6;    // one sixel char has 6 phases, one per row of the band

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
  out += "\"1;1;";
  append_int(out, width);
  out += ';';
  append_int(out, height);
}

void append_palette(std::string &out, std::vector<RGB> const &palette) {
  for (auto i = 0; i < int(palette.size()); ++i) {
    out += '#';
    append_int(out, i);
    out += ';';
    append_int(out, palette[i].red);
    out += ';';
    append_int(out, palette[i].green);
    out += ';';
    append_int(out, palette[i].blue);
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

// Builds a per-band palette with at most MAX_PALETTE_SIZE entries, reducing
// the color resolution until it fits. Returns the number of bits used.
int build_palette(const uint8_t *rgb, int width, int band_y, int band_height, int stride, std::vector<RGB> &palette, std::unordered_map<uint32_t, int> &index_map) {
  for (auto bits = 5; bits >= 2; --bits) {
    palette.clear();
    index_map.clear();

    for (auto y = band_y; y < band_y + band_height; ++y) {
      for (auto x = 0; x < width; ++x) {
        auto const *px = rgb + (y * stride + x) * 3;
        auto key = quantize(px, bits);
        if (index_map.find(key) == index_map.end()) {
          index_map.emplace(key, int(palette.size()));
          palette.push_back({ quantize_channel(px[0], bits), quantize_channel(px[1], bits), quantize_channel(px[2], bits) });
        }
      }
    }

    if (int(palette.size()) <= MAX_PALETTE_SIZE) {
      return bits;
    }
  }
  return 2;
}

// Classifies the columns of one band: the most frequent color of a column
// becomes its fill and the second most frequent color becomes the line drawn
// over it. Returns whether any column has a line. A sixel char carries a
// single color, so a column with more than two colors approximates the extra
// colors with the nearest of the two.
bool classify_band(const uint8_t *rgb, int width, int band_y, int band_height, int stride, int bits, std::vector<RGB> const &palette, std::unordered_map<uint32_t, int> const &index_map, std::vector<int> &fill, std::vector<int> &line, std::vector<uint8_t> &line_mask) {
  auto any_lines = false;
  auto counts = std::array<int, MAX_PALETTE_SIZE> { };

  for (auto x = 0; x < width; ++x) {
    counts.fill(0);
    auto column = std::array<int, MAX_BAND_COLORS> { };

    for (auto row = 0; row < band_height; ++row) {
      auto const *px = rgb + ((band_y + row) * stride + x) * 3;

      auto idx = 0;
      if (auto pos = index_map.find(quantize(px, bits)); pos != index_map.end()) {
        idx = pos->second;
      }

      column[row] = idx;
      ++counts[idx];
    }

    auto fill_idx = int(std::max_element(counts.begin(), counts.begin() + int(palette.size())) - counts.begin());

    auto line_idx = fill_idx;
    auto line_count = 0;
    for (auto i = 0; i < int(palette.size()); ++i) {
      if (i != fill_idx and counts[i] > line_count) {
        line_count = counts[i];
        line_idx = i;
      }
    }

    fill[x] = fill_idx;
    line[x] = line_idx;

    auto mask = uint8_t(0);
    for (auto row = 0; row < band_height; ++row) {
      if (column[row] == line_idx) {
        mask |= uint8_t(1 << row);
      }
    }
    line_mask[x] = mask;
    any_lines = any_lines or mask != 0;
  }

  return any_lines;
}

// Emits one band: the palette, the band advance, the opaque fills, then a `$`
// rewind to column 0 followed by the line pixels on top. Every phase emits
// exactly one data char per image column, so nothing shifts to the right.
void emit_band(std::string &out, const uint8_t *rgb, int width, int band_y, int band_height, int stride, std::vector<RGB> &palette, std::unordered_map<uint32_t, int> &index_map, std::vector<int> &fill, std::vector<int> &line, std::vector<uint8_t> &line_mask) {
  auto bits = build_palette(rgb, width, band_y, band_height, stride, palette, index_map);

  fill.assign(width, 0);
  line.assign(width, 0);
  line_mask.assign(width, 0);
  classify_band(rgb, width, band_y, band_height, stride, bits, palette, index_map, fill, line, line_mask);

  append_palette(out, palette);

  if (band_y > 0) {
    // Carriage return, then advance to the next band row.
    out += "$-";
  }

  // Fills: every column is painted with its fill color on all band rows.
  auto current_color = -1;
  auto fill_char = char(0x3F + (1 << band_height) - 1);
  for (auto x = 0; x < width;) {
    auto color = fill[x];
    auto start = x;
    while (x < width and fill[x] == color) {
      ++x;
    }
    if (color != current_color) {
      out += '#';
      append_int(out, color);
      current_color = color;
    }
    append_run(out, x - start, fill_char);
  }

  // Rewind to column 0 of the same band and draw the lines over the fills;
  // a zero char is transparent, leaving the fill pixels in place.
  out += '$';

  current_color = -1;
  for (auto x = 0; x < width;) {
    auto mask = line_mask[x];
    auto start = x;
    if (mask == 0) {
      while (x < width and line_mask[x] == 0) {
        ++x;
      }
      append_run(out, x - start, '?');
    } else {
      auto color = line[x];
      while (x < width and line_mask[x] == mask and line[x] == color) {
        ++x;
      }
      if (color != current_color) {
        out += '#';
        append_int(out, color);
        current_color = color;
      }
      append_run(out, x - start, char(0x3F + mask));
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
  auto fill = std::vector<int> { };
  auto line = std::vector<int> { };
  auto line_mask = std::vector<uint8_t> { };

  // A column holding a line and a fill color cannot be drawn with a single
  // sixel char: each data char advances the pen one column, so a second char
  // would shift the rest of the band to the right. The band is therefore
  // emitted twice, with a `$` carriage return between the two runs: first the
  // fills, then the lines drawn over them. The whole image uses a transparent
  // background (P2=1), so a zero char in the line run leaves the fill pixel in
  // place. Both runs still advance exactly one column per image column.

  out += "\x1bP0;1;0q";
  append_raster_attributes(out, width, height);

  for (auto band_y = 0; band_y < height; band_y += BAND_HEIGHT) {
    auto band_height = std::min(BAND_HEIGHT, height - band_y);
    emit_band(out, rgb, width, band_y, band_height, stride, palette, index_map, fill, line, line_mask);
  }

  // ST: end of the DCS.
  out += "\x1b\\";
  return out;
}

}
