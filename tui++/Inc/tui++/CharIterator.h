#pragma once

#include <string>

#include <tui++/Char.h>

namespace tui {

class CharIterator;

constexpr CharIterator begin(const CharIterator &i);

constexpr CharIterator end(const CharIterator &i);

class CharIterator {
  const char *utf8;
  std::size_t utf8_size;
  std::size_t utf8_index;

  char32_t code_point = 0;
  std::size_t code_point_byte_size = 0;

private:
  constexpr size_t get_code_point_size() const {
    return util::next_code_point(this->utf8, this->utf8_size, this->utf8_index, &const_cast<CharIterator*>(this)->code_point) - this->utf8_index;
  }

  constexpr void next() {
    if (this->code_point_byte_size) {
      this->utf8_index += this->code_point_byte_size;
      this->code_point_byte_size = 0;
    } else {
      this->utf8_index += get_code_point_size();
    }
  }

  constexpr void prev() {
    this->code_point_byte_size = this->utf8_index - util::prev_code_point(this->utf8, this->utf8_size, this->utf8_index, &this->code_point);
    this->utf8_index -= this->code_point_byte_size;
  }

  friend constexpr CharIterator begin(const CharIterator &i);

  friend constexpr CharIterator end(const CharIterator &i);

public:
  constexpr CharIterator(const char *utf8, size_t size, size_t index = 0) :
      utf8(utf8), utf8_size(size), utf8_index(index) {
  }

  constexpr CharIterator(const CharIterator&) = default;
  constexpr CharIterator(CharIterator&&) = default;

  CharIterator& operator=(const CharIterator&) = delete;
  CharIterator& operator=(CharIterator&&) = delete;

public:
  constexpr bool operator==(const CharIterator &other) const {
    return this->utf8_index == other.utf8_index and this->utf8 == other.utf8;
  }

  constexpr bool operator!=(const CharIterator &other) const {
    return this->utf8_index != other.utf8_index or this->utf8 != other.utf8;
  }

  constexpr CharIterator& operator++() {
    next();
    return *this;
  }

  constexpr CharIterator operator++(int) {
    auto utf8_index = this->utf8_index;
    next();
    return {this->utf8, this->utf8_size, utf8_index};
  }

  constexpr CharIterator& operator--() {
    prev();
    return *this;
  }

  constexpr CharIterator operator--(int) {
    auto utf8_index = this->utf8_index;
    prev();
    return {this->utf8, this->utf8_size, utf8_index};
  }

  constexpr Char operator*() const {
    if (not this->code_point_byte_size) {
      const_cast<CharIterator*>(this)->code_point_byte_size = get_code_point_size();
    }
    return {this->utf8 + this->utf8_index, this->code_point_byte_size, this->code_point};
  }
};

constexpr CharIterator begin(const CharIterator &i) {
  return i;
}

constexpr CharIterator end(const CharIterator &i) {
  return {const_cast<char*>(i.utf8), i.utf8_size, i.utf8_size};
}

constexpr CharIterator to_chars(const char *utf8, std::size_t size, std::size_t index = 0) {
  return {utf8, size, index};
}

constexpr CharIterator to_chars(const std::string &utf8, std::size_t index = 0) {
  return to_chars(utf8.data(), utf8.size(), index);
}

constexpr CharIterator to_chars(const std::string_view &utf8, std::size_t index = 0) {
  return to_chars(utf8.data(), utf8.size(), index);
}

}
