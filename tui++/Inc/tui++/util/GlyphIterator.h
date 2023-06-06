#pragma once

#include <string>

#include <tui++/util/utf-8.h>

namespace tui::util {

class GlyphIterator;

constexpr GlyphIterator begin(const GlyphIterator &i);

constexpr GlyphIterator end(const GlyphIterator &i);

class GlyphIterator {
  const char *utf8;
  std::size_t utf8_size;
  std::size_t utf8_index;

private:
  void next() {
    this->utf8_index = glyph_next(this->utf8, this->utf8_size, this->utf8_index);
  }

  void prev() {
    this->utf8_index = glyph_prev(this->utf8, this->utf8_size, this->utf8_index);
  }

  friend constexpr GlyphIterator begin(const GlyphIterator &i);

  friend constexpr GlyphIterator end(const GlyphIterator &i);

public:
  constexpr GlyphIterator(const char *utf8, size_t size, size_t index = 0) :
      utf8(utf8), utf8_size(size), utf8_index(index) {
  }

  constexpr GlyphIterator(const GlyphIterator&) = default;
  constexpr GlyphIterator(GlyphIterator&&) = default;

  GlyphIterator& operator=(const GlyphIterator&) = delete;
  GlyphIterator& operator=(GlyphIterator&&) = delete;

public:
  constexpr bool operator==(const GlyphIterator &other) const {
    return this->utf8_index == other.utf8_index and this->utf8 == other.utf8;
  }

  constexpr bool operator!=(const GlyphIterator &other) const {
    return this->utf8_index != other.utf8_index or this->utf8 != other.utf8;
  }

  GlyphIterator& operator++() {
    next();
    return *this;
  }

  GlyphIterator operator++(int) {
    auto utf8_index = this->utf8_index;
    next();
    return {this->utf8, this->utf8_size, utf8_index};
  }

  GlyphIterator& operator--() {
    prev();
    return *this;
  }

  GlyphIterator operator--(int) {
    auto utf8_index = this->utf8_index;
    prev();
    return {this->utf8, this->utf8_size, utf8_index};
  }

  const std::string_view operator*() const {
    auto width = glyph_width(this->utf8 + this->utf8_index, this->utf8_size - this->utf8_index);
    return {this->utf8 + this->utf8_index, width};
  }
};

constexpr GlyphIterator begin(const GlyphIterator &i) {
  return i;
}

constexpr GlyphIterator end(const GlyphIterator &i) {
  return {const_cast<char*>(i.utf8), i.utf8_size, i.utf8_size};
}

constexpr GlyphIterator glyphs(const char *utf8, std::size_t size, std::size_t index = 0) {
  return {utf8, size, index};
}

constexpr GlyphIterator glyphs(const std::string &utf8, std::size_t index = 0) {
  return glyphs(utf8.data(), utf8.size(), index);
}

constexpr GlyphIterator glyphs(const std::string_view &utf8, std::size_t index = 0) {
  return glyphs(utf8.data(), utf8.size(), index);
}

}
