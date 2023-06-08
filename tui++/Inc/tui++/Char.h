#pragma once

#include <cstring>
#include <cstdint>
#include <iostream>

namespace tui {

// UTF-8 mutibyte character
class Char {
  constexpr static size_t MAX_MB_CHAR_LEN = 4;

  union {
    struct {
      struct {
        uint32_t code :29;
        uint32_t len :3;
      };
      char bytes[4]; // utf-8
    };
    uint64_t u64;
  };

  friend std::ostream& operator<<(std::ostream &os, const Char &c) {
    return os << std::string_view { c.bytes, c.len };
  }

public:
  constexpr Char() = default;
  constexpr Char(const Char&) = default;
  constexpr Char(Char&&) = default;

  constexpr Char(char c) :
      bytes { } {
    this->bytes[0] = c;
    this->len = 1;
    this->code = 0;
  }

  constexpr Char(const char *s) :
      bytes { } {
    auto length = std::min(MAX_MB_CHAR_LEN, std::char_traits<char>::length(s));
    std::char_traits<char>::copy(this->bytes, s, length);
    this->len = length;
    this->code = 0;
  }

  constexpr Char(const std::string &s) :
      bytes { } {
    auto length = std::min(MAX_MB_CHAR_LEN, s.length());
    std::char_traits<char>::copy(this->bytes, s.data(), length);
    this->len = length;
    this->code = 0;
  }

  constexpr Char(const std::string_view &s) :
      bytes { } {
    auto length = std::min(MAX_MB_CHAR_LEN, s.length());
    std::char_traits<char>::copy(this->bytes, s.data(), length);
    this->len = length;
    this->code = 0;
  }

  constexpr Char& operator=(const Char&) = default;
  constexpr Char& operator=(Char&&) = default;

  constexpr operator std::string_view() const {
    return {this->bytes, this->len};
  }

  constexpr bool operator==(const Char &other) const {
    return this->u64 == other.u64;
  }

  constexpr bool operator!=(const Char &other) const {
    return this->u64 != other.u64;
  }
};

static_assert(sizeof(Char) == 8);

}

namespace tui::BoxDrawing {

constexpr Char DOWN_AND_RIGHT_LIGHT = "┌";
constexpr Char DOWN_AND_RIGHT_HEAVY = "┏";
constexpr Char DOWN_AND_RIGHT_DOUBLE = "╔";
constexpr Char DOWN_AND_RIGHT_ARC = "╭";
constexpr Char DOWN_AND_LEFT_LIGHT = "┐";
constexpr Char DOWN_AND_LEFT_HEAVY = "┓";
constexpr Char DOWN_AND_LEFT_DOUBLE = "╗";
constexpr Char DOWN_AND_LEFT_ARC = "╮";
constexpr Char UP_AND_RIGHT_LIGHT = "└";
constexpr Char UP_AND_RIGHT_HEAVY = "┗";
constexpr Char UP_AND_RIGHT_DOUBLE = "╚";
constexpr Char UP_AND_RIGHT_ARC = "╰";
constexpr Char UP_AND_LEFT_LIGHT = "┘";
constexpr Char UP_AND_LEFT_HEAVY = "┛";
constexpr Char UP_AND_LEFT_DOUBLE = "╝";
constexpr Char UP_AND_LEFT_ARC = "╯";
constexpr Char VERTICAL_LIGHT = "│";
constexpr Char VERTICAL_HEAVY = "┃";
constexpr Char VERTICAL_DOUBLE = "║";
constexpr Char VERTICAL_LIGHT_DOUBLE_DASH = "╎";
constexpr Char VERTICAL_HEAVY_DOUBLE_DASH = "╏";
constexpr Char VERTICAL_LIGHT_TRIPLE_DASH = "┆";
constexpr Char VERTICAL_HEAVY_TRIPLE_DASH = "┇";
constexpr Char VERTICAL_LIGHT_QUADRUPLE_DASH = "┊";
constexpr Char VERTICAL_HEAVY_QUADRUPLE_DASH = "┋";

constexpr Char HORIZONTAL_LIGHT = "─";
constexpr Char HORIZONTAL_HEAVY = "━";
constexpr Char HORIZONTAL_DOUBLE = "═";
constexpr Char HORIZONTAL_LIGHT_DOUBLE_DASH = "╌";
constexpr Char HORIZONTAL_HEAVY_DOUBLE_DASH = "╍";
constexpr Char HORIZONTAL_LIGHT_TRIPLE_DASH = "┄";
constexpr Char HORIZONTAL_HEAVY_TRIPLE_DASH = "┅";
constexpr Char HORIZONTAL_LIGHT_QUADRUPLE_DASH = "┈";
constexpr Char HORIZONTAL_HEAVY_QUADRUPLE_DASH = "┉";

}
