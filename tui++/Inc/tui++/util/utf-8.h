#pragma once

#include <string>

namespace tui::util {

using u8string = std::string;

char32_t mb_to_c32(const char *utf8, std::size_t size);
u8string c32_to_mb(char32_t c);

std::size_t glyph_width(const char *utf8, std::size_t size);
inline std::size_t glyph_width(const std::string &utf8) {
  return glyph_width(utf8.data(), utf8.size());
}
inline std::size_t glyph_width(const std::string_view &utf8) {
  return glyph_width(utf8.data(), utf8.size());
}

std::size_t glyph_next(const char *utf8, std::size_t size, std::size_t index);
std::size_t glyph_prev(const char *utf8, std::size_t size, std::size_t index);

}
