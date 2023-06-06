#pragma once

#include <cstring>

namespace tui {

struct Char {
  constexpr static size_t MAX_MB_CHAR_LEN = 4;

  char utf8[8];

  constexpr Char() = default;
  constexpr Char(const Char&) = default;
  constexpr Char(Char&&) = default;

  constexpr Char(const char *s) :
      utf8 { } {
    *this = s;
  }

  constexpr Char(const std::string &s) :
      utf8 { } {
    *this = s;
  }

  constexpr Char(const std::string_view &s) :
      utf8 { } {
    *this = s;
  }

  constexpr Char& operator=(const Char&) = default;
  constexpr Char& operator=(Char&&) = default;

  constexpr Char& operator=(const char *s) {
    auto length = std::min(MAX_MB_CHAR_LEN, std::char_traits<char>::length(s));
    std::char_traits<char>::copy(this->utf8, s, length);
    this->utf8[length] = 0;
    return *this;
  }

  constexpr Char& operator=(const std::string &s) {
    auto length = std::min(MAX_MB_CHAR_LEN, s.length());
    std::char_traits<char>::copy(this->utf8, s.data(), length);
    this->utf8[length] = 0;
    return *this;
  }

  constexpr Char& operator=(const std::string_view &s) {
    auto length = std::min(MAX_MB_CHAR_LEN, s.length());
    std::char_traits<char>::copy(this->utf8, s.data(), length);
    this->utf8[length] = 0;
    return *this;
  }
};

}

namespace tui::BoxDrawing {

constexpr Char VERTICAL_LIGHT { "│" };
constexpr Char VERTICAL_HEAVY { "┃" };
constexpr Char VERTICAL_LIGHT_DOUBLE_DASH { "╎" };
constexpr Char VERTICAL_HEAVY_DOUBLE_DASH { "╏" };
constexpr Char VERTICAL_LIGHT_TRIPLE_DASH { "┆" };
constexpr Char VERTICAL_HEAVY_TRIPLE_DASH { "┇" };
constexpr Char VERTICAL_LIGHT_QUADRUPLE_DASH { "┊" };
constexpr Char VERTICAL_HEAVY_QUADRUPLE_DASH { "┋" };

constexpr Char HORIZONTAL_LIGHT { "─" };
constexpr Char HORIZONTAL_HEAVY { "━" };
constexpr Char HORIZONTAL_LIGHT_DOUBLE_DASH { "╌" };
constexpr Char HORIZONTAL_HEAVY_DOUBLE_DASH { "╍" };
constexpr Char HORIZONTAL_LIGHT_TRIPLE_DASH { "┄" };
constexpr Char HORIZONTAL_HEAVY_TRIPLE_DASH { "┅" };
constexpr Char HORIZONTAL_LIGHT_QUADRUPLE_DASH { "┈" };
constexpr Char HORIZONTAL_HEAVY_QUADRUPLE_DASH { "┉" };
}

