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

  // Lead-byte classification (constexpr, so asserted at compile time):
  // ASCII is one byte, the multi-byte lead ranges two to four. Continuation
  // bytes (0x80-0xBF) and lead bytes above 0xF7 classify as 2/4 -- the
  // stream's own arithmetic -- and the decoder reports them invalid.
  static_assert(utf8_sequence_length(0x00) == 1);
  static_assert(utf8_sequence_length(0x7F) == 1);
  static_assert(utf8_sequence_length(0x80) == 2);
  static_assert(utf8_sequence_length(0xC2) == 2);
  static_assert(utf8_sequence_length(0xE0) == 3);
  static_assert(utf8_sequence_length(0xF0) == 4);
  static_assert(utf8_sequence_length(0xFF) == 4);

  // Per-character decode used by the text area's windows: the byte length
  // (capped at what is available) and the code point, 0 when the bytes are
  // cut short or invalid.
  {
    auto code = char32_t { };
    assert(utf8_char_decode("A", 1, &code) == 1 and code == 'A');
    assert(utf8_char_decode("测", 3, &code) == 3 and code == 0x6D4B);

    // A window that cuts a sequence reports what is left and decodes nothing.
    auto cut = char32_t { 0xABCD };
    assert(utf8_char_decode("测", 2, &cut) == 2 and cut == 0);
    assert(utf8_char_decode("测", 1, &cut) == 1 and cut == 0);

    // A stray continuation byte: two bytes consumed (its own legacy length),
    // nothing decoded.
    assert(utf8_char_decode("\x80x", 2, &cut) == 2 and cut == 0);
    assert(utf8_char_length("测", 0) == 0);
  }

  // Single code point to UTF-8 and back; values above U+10FFFF are not
  // representable and encode to nothing.
  {
    auto round_trip = [](char32_t cp) {
      auto s = to_utf8(cp);
      auto back = char32_t { };
      return mb_to_c32(s.data(), s.size(), &back) == (int) s.size() and back == cp;
    };
    assert(round_trip('A'));
    assert(round_trip(0x7FF));
    assert(round_trip(0x800));
    assert(round_trip(0xFFFF));
    assert(round_trip(0x10000));
    assert(round_trip(0x10FFFF));
    assert(to_utf8(char32_t(0x110000)).empty());
  }

  // The SWAR width path must agree with the per-character reading on long
  // ASCII runs (including the ASCII controls: C0/DEL take no cell, line feed
  // takes one) and on text that mixes ASCII with wide, combining and astral
  // characters.
  {
    auto ascii = std::string(300, 'a');
    ascii[100] = '\t'; // a C0 control: no cell
    ascii[200] = '\n'; // line feed takes one cell
    ascii[250] = char(0x7F); // DEL: no cell
    auto fast = glyph_width(ascii.data(), ascii.size());
    auto scalar = detail::scalar_glyph_width(ascii.data(), ascii.size());
    assert(fast == scalar);
    assert(glyph_width(ascii) == 298);

    auto mixed = "ascii prefix 测 wide ⚫ combining a\xCC\x81 tail"s;
    assert(glyph_width(mixed) == 40);

    assert(ascii_prefix_length("abc", 3) == 3);
    assert(ascii_prefix_length("abc\xC2\xA2def", 8) == 3);
    assert(ascii_prefix_length("\x80", 1) == 0);
    assert(ascii_prefix_length(std::string(20, 'x').data(), 20) == 20);

    // Control runs that end right at an eight-byte SWAR step (7 and 9 cell
    // runs around the boundary) and multi-byte characters must keep the run
    // counting in step with the per-character reading.
    auto tricky = std::string { };
    tricky.append(7, '\x01');
    tricky.append(9, 'x');
    tricky += "\xC2\xA2"; // cent sign: one cell
    tricky.append(6, '\x7F');
    tricky.append(10, 'y');
    assert(glyph_width(tricky.data(), tricky.size()) == detail::scalar_glyph_width(tricky.data(), tricky.size()));
    assert(glyph_width(tricky) == 20); // 9 x + 1 cent + 10 y
  }
}
