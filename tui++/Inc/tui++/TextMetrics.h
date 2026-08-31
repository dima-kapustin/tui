#pragma once

#include <tui++/Font.h>
#include <tui++/Dimension.h>

#include <tui++/util/utf-8.h>
#include <tui++/util/unicode.h>

#include <algorithm>
#include <string_view>

namespace tui {

// Size of one terminal cell in the "graphic" (sixel) renderer, in pixels.
// Kept in sync with GraphicScreen::CELL_WIDTH / CELL_HEIGHT.
constexpr int GRAPHIC_CELL_WIDTH = 8;
constexpr int GRAPHIC_CELL_HEIGHT = 16;

// Measures the dimensions of a (possibly multi-line) piece of text in the
// screen's units: terminal cells for the text screen, pixels for the graphic
// screen. The font is passed in at construction because the string size may
// depend on font properties.
class TextMetrics {
public:
  TextMetrics(const Font &font) :
      font(font) {
  }

  virtual ~TextMetrics() = default;

  // Width of the widest line of `text`.
  virtual int get_width(std::string_view text) const = 0;

  // Total height of `text` (line count times the line height); 0 for empty.
  virtual int get_height(std::string_view text) const = 0;

  // Height of a single line.
  virtual int get_line_height() const = 0;

  // Width of a single character.
  virtual int get_char_width(char32_t ch) const = 0;

  Dimension get_size(std::string_view text) const {
    return { get_width(text), get_height(text) };
  }

  const Font& get_font() const {
    return this->font;
  }

protected:
  Font font;
};

namespace detail {

inline std::size_t count_lines(std::string_view text) {
  auto lines = std::size_t(1);
  for (auto ch : text) {
    if (ch == '\n') {
      ++lines;
    }
  }
  return lines;
}

// Width, in glyphs, of the widest line of `text`.
inline std::size_t max_line_glyph_width(std::string_view text) {
  if (text.empty()) {
    return 0;
  }
  auto max_width = std::size_t(0);
  auto pos = std::size_t(0);
  while (true) {
    auto start = pos;
    while (pos < text.size() and text[pos] != '\n') {
      ++pos;
    }
    max_width = std::max(max_width, util::glyph_width(text.substr(start, pos - start)));
    if (pos == text.size()) {
      break;
    }
    ++pos;
  }
  return max_width;
}

}

// Measures text in terminal cells (one unit per character cell).
class CellTextMetrics: public TextMetrics {
public:
  using TextMetrics::TextMetrics;

  virtual int get_char_width(char32_t ch) const override {
    return util::unicode::glyph_width(ch);
  }

  virtual int get_line_height() const override {
    return 1;
  }

  virtual int get_width(std::string_view text) const override {
    return int(detail::max_line_glyph_width(text));
  }

  virtual int get_height(std::string_view text) const override {
    return text.empty() ? 0 : int(detail::count_lines(text) * get_line_height());
  }
};

// Measures text in pixels, one terminal cell being GRAPHIC_CELL_WIDTH x
// GRAPHIC_CELL_HEIGHT pixels.
class PixelTextMetrics: public TextMetrics {
public:
  PixelTextMetrics(const Font &font) :
      TextMetrics(font) {
  }

  virtual int get_char_width(char32_t ch) const override {
    return util::unicode::glyph_width(ch) * GRAPHIC_CELL_WIDTH;
  }

  virtual int get_line_height() const override {
    return GRAPHIC_CELL_HEIGHT;
  }

  virtual int get_width(std::string_view text) const override {
    return int(detail::max_line_glyph_width(text) * GRAPHIC_CELL_WIDTH);
  }

  virtual int get_height(std::string_view text) const override {
    return text.empty() ? 0 : int(detail::count_lines(text) * get_line_height());
  }
};

}
