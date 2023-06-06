#include <tui++/util/GlyphIterator.h>

#include <cassert>

using namespace tui::util;
using namespace std::string_literals;

void test_GlyphIterator() {
  auto utf8 = "测试"s;
  auto glyphs = to_glyphs(utf8);

  ++glyphs;
  ++glyphs;

  assert(glyphs == end(glyphs));

  --glyphs;
  --glyphs;

  assert(glyphs == to_glyphs(utf8));
}
