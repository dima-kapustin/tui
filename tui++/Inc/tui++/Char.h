#pragma once

#include <cstring>
#include <cstdint>
#include <iostream>

#include <tui++/util/utf-8.h>
#include <tui++/util/unicode.h>

namespace tui {

enum class CharCode : char32_t {
  DOWN_AND_RIGHT_LIGHT = L'┌',
  DOWN_AND_RIGHT_HEAVY = L'┏',
  DOWN_AND_RIGHT_DOUBLE = L'╔',
  DOWN_AND_RIGHT_ARC = L'╭',
  DOWN_AND_LEFT_LIGHT = L'┐',
  DOWN_AND_LEFT_HEAVY = L'┓',
  DOWN_AND_LEFT_DOUBLE = L'╗',
  DOWN_AND_LEFT_ARC = L'╮',
  UP_AND_RIGHT_LIGHT = L'└',
  UP_AND_RIGHT_HEAVY = L'┗',
  UP_AND_RIGHT_DOUBLE = L'╚',
  UP_AND_RIGHT_ARC = L'╰',
  UP_AND_LEFT_LIGHT = L'┘',
  UP_AND_LEFT_HEAVY = L'┛',
  UP_AND_LEFT_DOUBLE = L'╝',
  UP_AND_LEFT_ARC = L'╯',
  VERTICAL_LIGHT = L'│',
  VERTICAL_HEAVY = L'┃',
  VERTICAL_DOUBLE = L'║',
  VERTICAL_LIGHT_DOUBLE_DASH = L'╎',
  VERTICAL_HEAVY_DOUBLE_DASH = L'╏',
  VERTICAL_LIGHT_TRIPLE_DASH = L'┆',
  VERTICAL_HEAVY_TRIPLE_DASH = L'┇',
  VERTICAL_LIGHT_QUADRUPLE_DASH = L'┊',
  VERTICAL_HEAVY_QUADRUPLE_DASH = L'┋',

  HORIZONTAL_LIGHT = L'─',
  HORIZONTAL_HEAVY = L'━',
  HORIZONTAL_DOUBLE = L'═',
  HORIZONTAL_LIGHT_DOUBLE_DASH = L'╌',
  HORIZONTAL_HEAVY_DOUBLE_DASH = L'╍',
  HORIZONTAL_LIGHT_TRIPLE_DASH = L'┄',
  HORIZONTAL_HEAVY_TRIPLE_DASH = L'┅',
  HORIZONTAL_LIGHT_QUADRUPLE_DASH = L'┈',
  HORIZONTAL_HEAVY_QUADRUPLE_DASH = L'┉',
};

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

  constexpr Char(char32_t c) :
      bytes { } {
    auto mb = util::c32_to_mb(c);
    auto length = std::min(MAX_BYTE_LEN, mb.length());
    std::char_traits<char>::copy(this->bytes, mb.data(), length);
    this->len = length - 1;
    this->code = c;
  }

  constexpr Char(CharCode c) :
      Char(char32_t(c)) {
  }

public:
  constexpr Char& operator=(const Char&) = default;
  constexpr Char& operator=(Char&&) = default;

public:
  constexpr operator std::string_view() const {
    return {this->bytes, size_t(this->len) + 1};
  }

  constexpr int compare(const Char &other) const {
    auto length = std::min(size_t(this->len), size_t(other.len)) + 1;
    return std::char_traits<char>::compare(this->bytes, other.bytes, length);
  }

  constexpr bool operator<(const Char &other) const {
    return compare(other) < 0;
  }

  constexpr bool operator==(const Char &other) const {
    return this->u64 == other.u64;
  }

  constexpr bool operator!=(const Char &other) const {
    return this->u64 != other.u64;
  }

  constexpr size_t byte_length() const {
    return size_t(this->len) + 1;
  }

  constexpr int glyph_width() const {
    return util::unicode::glyph_width(char32_t(this->code));
  }
};

static_assert(sizeof(Char) == 8);

constexpr std::ostream& operator<<(std::ostream &os, const Char &c) {
  return os << std::string_view(c);
}

}

namespace tui::BoxDrawing {

constexpr Char DOWN_AND_RIGHT_LIGHT = CharCode::DOWN_AND_RIGHT_LIGHT;
constexpr Char DOWN_AND_RIGHT_HEAVY = CharCode::DOWN_AND_RIGHT_HEAVY;
constexpr Char DOWN_AND_RIGHT_DOUBLE = CharCode::DOWN_AND_RIGHT_DOUBLE;
constexpr Char DOWN_AND_RIGHT_ARC = CharCode::DOWN_AND_RIGHT_ARC;
constexpr Char DOWN_AND_LEFT_LIGHT = CharCode::DOWN_AND_LEFT_LIGHT;
constexpr Char DOWN_AND_LEFT_HEAVY = CharCode::DOWN_AND_LEFT_HEAVY;
constexpr Char DOWN_AND_LEFT_DOUBLE = CharCode::DOWN_AND_LEFT_DOUBLE;
constexpr Char DOWN_AND_LEFT_ARC = CharCode::DOWN_AND_LEFT_ARC;
constexpr Char UP_AND_RIGHT_LIGHT = CharCode::UP_AND_RIGHT_LIGHT;
constexpr Char UP_AND_RIGHT_HEAVY = CharCode::UP_AND_RIGHT_HEAVY;
constexpr Char UP_AND_RIGHT_DOUBLE = CharCode::UP_AND_RIGHT_DOUBLE;
constexpr Char UP_AND_RIGHT_ARC = CharCode::UP_AND_RIGHT_ARC;
constexpr Char UP_AND_LEFT_LIGHT = CharCode::UP_AND_LEFT_LIGHT;
constexpr Char UP_AND_LEFT_HEAVY = CharCode::UP_AND_LEFT_HEAVY;
constexpr Char UP_AND_LEFT_DOUBLE = CharCode::UP_AND_LEFT_DOUBLE;
constexpr Char UP_AND_LEFT_ARC = CharCode::UP_AND_LEFT_ARC;
constexpr Char VERTICAL_LIGHT = CharCode::VERTICAL_LIGHT;
constexpr Char VERTICAL_HEAVY = CharCode::VERTICAL_HEAVY;
constexpr Char VERTICAL_DOUBLE = CharCode::VERTICAL_DOUBLE;
constexpr Char VERTICAL_LIGHT_DOUBLE_DASH = CharCode::VERTICAL_LIGHT_DOUBLE_DASH;
constexpr Char VERTICAL_HEAVY_DOUBLE_DASH = CharCode::VERTICAL_HEAVY_DOUBLE_DASH;
constexpr Char VERTICAL_LIGHT_TRIPLE_DASH = CharCode::VERTICAL_LIGHT_TRIPLE_DASH;
constexpr Char VERTICAL_HEAVY_TRIPLE_DASH = CharCode::VERTICAL_HEAVY_TRIPLE_DASH;
constexpr Char VERTICAL_LIGHT_QUADRUPLE_DASH = CharCode::VERTICAL_LIGHT_QUADRUPLE_DASH;
constexpr Char VERTICAL_HEAVY_QUADRUPLE_DASH = CharCode::VERTICAL_HEAVY_QUADRUPLE_DASH;

constexpr Char HORIZONTAL_LIGHT = CharCode::HORIZONTAL_LIGHT;
constexpr Char HORIZONTAL_HEAVY = CharCode::HORIZONTAL_HEAVY;
constexpr Char HORIZONTAL_DOUBLE = CharCode::HORIZONTAL_DOUBLE;
constexpr Char HORIZONTAL_LIGHT_DOUBLE_DASH = CharCode::HORIZONTAL_LIGHT_DOUBLE_DASH;
constexpr Char HORIZONTAL_HEAVY_DOUBLE_DASH = CharCode::HORIZONTAL_HEAVY_DOUBLE_DASH;
constexpr Char HORIZONTAL_LIGHT_TRIPLE_DASH = CharCode::HORIZONTAL_LIGHT_TRIPLE_DASH;
constexpr Char HORIZONTAL_HEAVY_TRIPLE_DASH = CharCode::HORIZONTAL_HEAVY_TRIPLE_DASH;
constexpr Char HORIZONTAL_LIGHT_QUADRUPLE_DASH = CharCode::HORIZONTAL_LIGHT_QUADRUPLE_DASH;
constexpr Char HORIZONTAL_HEAVY_QUADRUPLE_DASH = CharCode::HORIZONTAL_HEAVY_QUADRUPLE_DASH;

}
