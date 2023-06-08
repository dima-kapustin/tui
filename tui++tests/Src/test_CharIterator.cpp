#include <tui++/CharIterator.h>

#include <cassert>

using namespace tui;
using namespace std::string_literals;

void test_CharIterator() {
  auto utf8 = "测试"s;
  auto glyphs = to_chars(utf8);

  ++glyphs;
  ++glyphs;

  assert(glyphs == end(glyphs));

  --glyphs;
  --glyphs;

  assert(glyphs == to_chars(utf8));
}
