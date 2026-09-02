#pragma once

#include <tui++/util/EnumFlags.h>

#include <string>
#include <utility>

namespace tui {

/**
 * A font: family, size and style, mirroring Swing's Font granularity.
 *
 * On the graphic (sixel) screen the size is the glyph width in pixels: the
 * 16x32 raster glyphs of the embedded font scale to `size` x `2*size` pixels,
 * so a size-16 font draws a glyph that exactly fills the 16x32 terminal cell.
 * The text screen measures in cells and ignores the size.
 *
 * The style combines Font::BOLD and Font::ITALIC; the graphic renderer
 * doubles every stroke for BOLD and shears the rows for ITALIC. Both the
 * size and the style are theme-configurable through the "defaultFont"
 * property (see Theme).
 */
class Font {
public:
  enum StyleFlags {
    PLAIN = 0,
    BOLD = 1,
    ITALIC = 2
  };

  using Style = util::EnumFlags<StyleFlags>;

  // The default size matches the 16x32 raster glyphs of the graphic renderer
  // one-to-one (16 pixels wide, 32 tall).
  constexpr static int DEFAULT_SIZE = 16;

private:
  std::string family = "Monospaced";
  int size = DEFAULT_SIZE;
  Style style = PLAIN;

public:
  Font() = default;

  Font(std::string family, int size, Style style = PLAIN) :
      family(std::move(family)), size(size), style(style) {
  }

  const std::string& get_family() const {
    return this->family;
  }

  int get_size() const {
    return this->size;
  }

  Style get_style() const {
    return this->style;
  }

  void set_family(std::string family) {
    this->family = std::move(family);
  }

  void set_size(int size) {
    this->size = size;
  }

  void set_style(Style style) {
    this->style = style;
  }

  bool operator==(const Font &other) const {
    return this->family == other.family and this->size == other.size and this->style == other.style;
  }

  bool operator!=(const Font &other) const {
    return not (*this == other);
  }
};

}
