#pragma once

#include <string>

#include <tui++/util/utf-8.h>

namespace tui::util {

template<typename T, typename Enable = void>
class GlyphIterator;

template<typename T>
constexpr GlyphIterator<T> begin(const GlyphIterator<T> &i);

template<typename T>
constexpr GlyphIterator<T> end(const GlyphIterator<T> &i);

template<>
class GlyphIterator<const std::string> {
protected:
  const std::string &utf8;
  std::size_t utf8_index;

protected:
  void next() {
    this->utf8_index = glyph_next(this->utf8.data(), this->utf8.size(), this->utf8_index);
  }

  void prev() {
    this->utf8_index = glyph_prev(this->utf8.data(), this->utf8.size(), this->utf8_index);
  }

private:
  template<typename T>
  friend constexpr GlyphIterator<T> begin(const GlyphIterator<T> &i);

  template<typename T>
  friend constexpr GlyphIterator<T> end(const GlyphIterator<T> &i);

public:
  GlyphIterator(const std::string &utf8) :
      utf8(utf8), utf8_index(0) {
  }

  GlyphIterator(const std::string &utf8, size_t index) :
      utf8(utf8), utf8_index(index) {
  }

  GlyphIterator(const GlyphIterator&) = default;
  GlyphIterator(GlyphIterator&&) = default;

  GlyphIterator& operator=(const GlyphIterator&) = delete;
  GlyphIterator& operator=(GlyphIterator&&) = delete;

public:
  constexpr bool operator==(const GlyphIterator &other) const {
    return this->utf8_index == other.utf8_index and &this->utf8 == &other.utf8;
  }

  constexpr bool operator!=(const GlyphIterator &other) const {
    return this->utf8_index != other.utf8_index or &this->utf8 != &other.utf8;
  }

  GlyphIterator& operator++() {
    next();
    return *this;
  }

  GlyphIterator operator++(int) {
    auto utf8_index = this->utf8_index;
    next();
    return {this->utf8, utf8_index};
  }

  GlyphIterator& operator--() {
    prev();
    return *this;
  }

  GlyphIterator operator--(int) {
    auto utf8_index = this->utf8_index;
    prev();
    return {this->utf8, utf8_index};
  }
};

template<>
class GlyphIterator<std::string> : public GlyphIterator<const std::string> {
  using base = GlyphIterator<const std::string>;

public:
  GlyphIterator(std::string &utf8) :
      base(utf8) {
  }

  GlyphIterator(const std::string &utf8, size_t index) :
      base(utf8, index) {
  }

  GlyphIterator& operator++() {
    base::operator ++();
    return *this;
  }

  GlyphIterator operator++(int) {
    auto utf8_index = this->utf8_index;
    next();
    return {const_cast<std::string&>(this->utf8), utf8_index};
  }

  GlyphIterator& operator--() {
    base::operator --();
    return *this;
  }

  GlyphIterator operator--(int) {
    auto utf8_index = this->utf8_index;
    prev();
    return {const_cast<std::string&>(this->utf8), utf8_index};
  }
};

template<typename T>
constexpr GlyphIterator<T> begin(const GlyphIterator<T> &i) {
  return i;
}

template<typename T>
constexpr GlyphIterator<T> end(const GlyphIterator<T> &i) {
  return {i.utf8, i.utf8.size()};
}

}
