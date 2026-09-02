#pragma once

#include <cstdint>
#include <string>

namespace tui {

// Encodes an RGB image into a complete DEC sixel escape sequence.
//
// This class is the only place that knows about the sixel protocol: the DCS
// wrapper, raster attributes, band palettes and the sixel data itself.
// Renderers just hand over pixels and get back a string that can be written
// to the terminal.
class SixelEncoder {
public:
  // Encodes a width x height image. The pixels are tightly packed rows of
  // 24-bit RGB (8 bits per channel), `stride` pixels per row.
  //
  // The image is emitted as a single transparent (P2=1) sixel image. Its
  // palette is defined once before the first band (terminals keep one palette
  // per image, so redefining entries mid-image would recolor earlier bands);
  // each band is painted with one pass per color, most frequent first, each
  // pass drawing only the rows where that color occurs with a `$` carriage
  // return between the passes, so every pixel keeps its exact color, is
  // painted exactly once, and no column shifts.
  static std::string encode(const uint8_t *rgb, int width, int height, int stride);
};

}
