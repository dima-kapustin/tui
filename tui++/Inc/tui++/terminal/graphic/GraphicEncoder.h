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
class GraphicEncoder {
public:
  // Encodes a width x height image. The pixels are tightly packed rows of
  // 24-bit RGB (8 bits per channel), `stride` pixels per row.
  //
  // The image is emitted as a single transparent (P2=1) sixel image: each
  // band is written twice with a `$` carriage return between the runs, first
  // the fills and then the lines over them, so every image column advances
  // the pen exactly once per run.
  static std::string encode(const uint8_t *rgb, int width, int height, int stride);
};

}
