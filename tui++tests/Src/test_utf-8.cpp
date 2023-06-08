#include <tui++/util/utf-8.h>

#include <cassert>

using namespace tui::util;
using namespace std::string_literals;

void test_utf8() {
  static_assert(glyph_width(""s) == 0);
  static_assert(glyph_width("a"s) == 1);
  static_assert(glyph_width("ab"s) == 2);
  static_assert(glyph_width("⬤"s) == 1);

  // Fullwidth glyphs:
  static_assert(glyph_width("测"s) == 2);
  static_assert(glyph_width("测试"s) == 4);
  static_assert(glyph_width("⚫"s) == 2);
  static_assert(glyph_width("🪐"s) == 2);

  // Combining characters:
  static_assert(glyph_width("ā"s) == 1);
  static_assert(glyph_width("a⃒"s) == 1);
  static_assert(glyph_width("a̗"s) == 1);

  // Control characters:
  static_assert(glyph_width("\1"s) == 0);
  static_assert(glyph_width("a\1a"s) == 2);
}
