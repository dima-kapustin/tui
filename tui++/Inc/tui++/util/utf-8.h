#pragma once

#include <string>
#include <tui++/util/unicode.h>

namespace tui::util {

using u8string = std::string;

constexpr int mb_to_c32(const char *utf8, std::size_t size, char32_t *c32) {
  if (size == 0 or *utf8 == 0) {
    return 0;
  }

  auto const c0 = utf8[0];

  // 1 byte code point
  if ((c0 & 0b1000'0000) == 0b0000'0000) {
    *c32 = c0 & 0b0111'1111;
    return 1;
  }

  // 2 byte code point
  if ((c0 & 0b1110'0000) == 0b1100'0000) {
    if (size >= 2) {
      auto const c1 = utf8[1];
      auto c = char32_t { 0 };
      c += c0 & 0b0001'1111;
      c <<= 6;
      c += c1 & 0b0011'1111;
      *c32 = c;
      return 2;
    } else {
      return -2; // incomplete
    }
  }

  // 3 byte code point
  if ((c0 & 0b1111'0000) == 0b1110'0000) {
    if (size >= 3) {
      auto const c1 = utf8[1];
      auto const c2 = utf8[2];
      auto c = char32_t { 0 };
      c += c0 & 0b0000'1111;
      c <<= 6;
      c += c1 & 0b0011'1111;
      c <<= 6;
      c += c2 & 0b0011'1111;
      *c32 = c;
      return 3;
    } else {
      return -2; // incomplete
    }
  }

  // 4 byte string.
  if ((c0 & 0b1111'1000) == 0b1111'0000) {
    if (size >= 4) {
      auto const c1 = utf8[1];
      auto const c2 = utf8[2];
      auto const c3 = utf8[3];
      auto c = char32_t { 0 };
      c += c0 & 0b0000'0111;
      c <<= 6;
      c += c1 & 0b0011'1111;
      c <<= 6;
      c += c2 & 0b0011'1111;
      c <<= 6;
      c += c3 & 0b0011'1111;
      *c32 = c;
      return 4;
    } else {
      return -2;
    }
  }

  return -1;
}

constexpr u8string c32_to_mb(char32_t c) {
  u8string mb(4, '\0');

  // 1 byte UTF8
  if (c <= 0b000'0000'0111'1111) {
    auto const b1 = c;
    mb[0] = u8string::value_type(b1);
    mb.resize(1);
    return mb;
  }

  // 2 bytes UTF8
  if (c <= 0b000'0111'1111'1111) {
    auto const b2 = c & 0b111111;
    c >>= 6;
    auto const b1 = c;
    mb[0] = u8string::value_type(0b11000000 + b1);
    mb[1] = u8string::value_type(0b10000000 + b2);
    mb.resize(2);
    return mb;
  }

  // 3 bytes UTF8
  if (c <= 0b1111'1111'1111'1111) {
    auto const b3 = c & 0b111111;
    c >>= 6;
    auto const b2 = c & 0b111111;
    c >>= 6;
    auto const b1 = c;
    mb[0] = u8string::value_type(0b11100000 + b1);
    mb[1] = u8string::value_type(0b10000000 + b2);
    mb[2] = u8string::value_type(0b10000000 + b3);
    mb.resize(3);
    return mb;
  }

  // 4 bytes UTF8
  if (c <= 0b1'0000'1111'1111'1111'1111) {
    auto const b4 = c & 0b111111;
    c >>= 6;
    auto const b3 = c & 0b111111;
    c >>= 6;
    auto const b2 = c & 0b111111;
    c >>= 6;
    auto const b1 = c;
    mb[0] = u8string::value_type(0b11110000 + b1);
    mb[1] = u8string::value_type(0b10000000 + b2);
    mb[2] = u8string::value_type(0b10000000 + b3);
    mb[3] = u8string::value_type(0b10000000 + b4);
    return mb;
  }
  return mb;
}

constexpr std::size_t glyph_width(const char *utf8, std::size_t size) {
  auto width = std::size_t { 0 };
  auto index = std::size_t { 0 };
  while (index < size) {
    auto cp = char32_t { };
    auto cp_size = mb_to_c32(utf8 + index, size - index, &cp);
    if (cp_size < 0) {
      index += 1;
      continue;
    } else if (unicode::is_full_width(cp)) {
      width += 2;
    } else if (not (unicode::is_control(cp) or unicode::is_combining(cp))) {
      width += 1;
    }
    index += cp_size;
  }
  return width;
}

constexpr std::size_t glyph_width(const std::string &utf8) {
  return glyph_width(utf8.data(), utf8.size());
}
constexpr std::size_t glyph_width(const std::string_view &utf8) {
  return glyph_width(utf8.data(), utf8.size());
}

constexpr std::size_t next_code_point(const char *utf8, std::size_t size, std::size_t index, char32_t *cp) {
  while (index < size) {
    auto cp_size = mb_to_c32(utf8 + index, size - index, cp);
    if (cp_size < 0) {
      index += 1;
    } else {
      index += cp_size;
      if (not (unicode::is_control(*cp) or unicode::is_combining(*cp))) {
        break;
      }
    }
  }
  return index;
}

constexpr std::size_t prev_code_point(const char *utf8, std::size_t size, std::size_t index, char32_t *cp) {
  while (true) {
    if (index == 0) {
      return 0;
    }
    index -= 1;
    auto cp_size = mb_to_c32(&utf8[index], size - index, cp);
    if (cp_size < 0) {
      index -= 1;
    } else {
      index -= cp_size;
      if (not (unicode::is_control(*cp) or unicode::is_combining(*cp))) {
        break;
      }
    }
  }
  return index;
}

}
