#pragma once

#include <cstring>
#include <cstdint>
#include <iostream>
#include <string_view>

#include <tui++/util/utf-8.h>
#include <tui++/util/unicode.h>

namespace tui {

class CharIterator;

// UTF-8 mutibyte character
class Char {
  constexpr static size_t MAX_BYTE_LEN = 4;

  union {
    struct {
      struct {
        uint32_t code :30;
        uint32_t len :2;
      };
      char bytes[MAX_BYTE_LEN]; // utf-8
    };
    uint64_t u64;
  };

private:
  constexpr Char(const char *bytes, size_t size, char32_t code) :
      bytes { } {
    std::char_traits<char>::copy(this->bytes, bytes, size);
    this->len = size - 1;
    this->code = code;
  }

  friend class CharIterator;

public:
  constexpr Char() = default;
  constexpr Char(const Char&) = default;
  constexpr Char(Char&&) = default;

  constexpr Char(char c) :
      bytes { } {
    this->bytes[0] = c;
    this->len = 0;
    this->code = c;
  }

  constexpr Char(wchar_t c) :
      Char(char32_t(c)) {
  }

  constexpr Char(char32_t c) :
      bytes { } {
    auto length = util::c32_to_mb(c, this->bytes);
    this->len = length - 1;
    this->code = c;
  }

public:
  constexpr Char& operator=(const Char&) = default;
  constexpr Char& operator=(Char&&) = default;

public:
  constexpr operator std::string_view() const {
    return {this->bytes, size_t(this->len) + 1};
  }

  constexpr int compare(const Char &other) const {
    return int(this->code - other.code);
  }

  constexpr bool operator<(const Char &other) const {
    return compare(other) < 0;
  }

  constexpr bool operator==(const Char &other) const {
    return this->code == other.code;
  }

  friend constexpr bool operator==(const Char &left, char32_t right) {
    return left.code == right;
  }

  constexpr size_t byte_length() const {
    return size_t(this->len) + 1;
  }

  constexpr int glyph_width() const {
    return util::unicode::glyph_width(char32_t(this->code));
  }

  constexpr char32_t get_code() const {
    return char32_t(this->code);
  }
};

static_assert(sizeof(Char) == sizeof(uint64_t));

constexpr std::ostream& operator<<(std::ostream &os, const Char &c) {
  return os << std::string_view(c);
}

}
