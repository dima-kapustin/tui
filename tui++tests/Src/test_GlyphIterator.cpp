#include <tui++/util/GlyphIterator.h>

#include <cassert>

using namespace tui::util;
using namespace std::string_literals;

void test_GlyphIterator() {
  auto utf8 = "测试"s;
  auto glyphs = GlyphIterator { utf8.data(), utf8.size() };

  ++glyphs;
  ++glyphs;

  assert(glyphs == end(glyphs));

  --glyphs;
  --glyphs;

  assert(glyphs == begin(GlyphIterator { utf8.data(), utf8.size() }));
}
