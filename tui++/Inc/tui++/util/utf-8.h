#pragma once

// UTF-8 helpers shared by the whole toolkit: multi-byte <-> code point
// conversion, sequence classification, cell-width measurement of UTF-8 text
// and wide/UTF-16 interop. Everything byte-level lives here; per-code-point
// classification (control/combining/full-width tables) lives in unicode.h.
//
// The hot text paths (text measurement, line painting) are dominated by
// ASCII, so glyph_width() scores whole ASCII runs eight bytes at a time with
// SWAR ("SIMD within a register", the technique glibc's strlen and StringZilla
// use) before falling back to the per-character decoder. The scalar decoders
// stay constexpr for the constant-expression paths (Char, static assertions).

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include <tui++/util/string.h>
#include <tui++/util/unicode.h>

namespace tui::util {

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
      return -2; // incomplete
    }
  }

  return -1; // invalid
}

constexpr int mb_to_c32(const u8string &str, char32_t *c32) {
  return mb_to_c32(str.data(), str.length(), c32);
}

constexpr int mb_to_c32(const u8string_view &str, char32_t *c32) {
  return mb_to_c32(str.data(), str.length(), c32);
}

// Byte length of the UTF-8 sequence a lead byte announces (1..4). Mirrors
// the stream's own arithmetic: continuation bytes (0x80-0xBF) and lead bytes
// above 0xF7 classify as 2/4 so callers never stall on malformed input; the
// decoder (mb_to_c32) reports those as invalid.
constexpr int utf8_sequence_length(std::uint8_t first) noexcept {
  if (first < 0x80) {
    return 1;
  }
  if (first < 0xE0) {
    return 2;
  }
  if (first < 0xF0) {
    return 3;
  }
  return 4;
}

// Length of the sequence starting at `utf8`, capped at the bytes available:
// a sequence a buffer cuts short (the end of a window, end of the document)
// reports the bytes that are left instead of over-reading.
constexpr int utf8_char_length(const char *utf8, std::size_t available) noexcept {
  if (available == 0) {
    return 0;
  }
  auto const length = std::size_t(utf8_sequence_length(std::uint8_t(utf8[0])));
  return int(std::min(length, available));
}

// Decodes the sequence starting at `utf8` and returns its (capped) byte
// length. `*code` receives the code point, or 0 when the bytes do not form a
// complete, valid sequence.
constexpr int utf8_char_decode(const char *utf8, std::size_t available, char32_t *code) noexcept {
  auto const length = utf8_char_length(utf8, available);
  if (length == 0 or mb_to_c32(utf8, std::size_t(length), code) <= 0) {
    *code = 0;
  }
  return length;
}

constexpr size_t c32_to_mb(char32_t c, char *mb) {
  // 1 byte UTF8
  if (c <= 0b000'0000'0111'1111) {
    auto const b1 = c;
    mb[0] = u8string::value_type(b1);
    return 1;
  }

  // 2 bytes UTF8
  if (c <= 0b000'0111'1111'1111) {
    auto const b2 = c & 0b111111;
    c >>= 6;
    auto const b1 = c;
    mb[0] = u8string::value_type(0b11000000 + b1);
    mb[1] = u8string::value_type(0b10000000 + b2);
    return 2;
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
    return 3;
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
    return 4;
  }
  return 0;
}

namespace detail {

constexpr std::uint64_t ones64() noexcept {
  return 0x0101'0101'0101'0101ull;
}

constexpr std::uint64_t high_bits64() noexcept {
  return 0x8080'8080'8080'8080ull;
}

constexpr bool has_high_bit(std::uint64_t chunk) noexcept {
  return bool(chunk & high_bits64());
}

// One high bit per byte lane whose byte lies in [first, last). Per-lane
// exact for all-ASCII chunks: b >= n  <=>  b + (0x80 - n) >= 0x80, and the
// per-lane sums never reach 0x100 (max 0x7F + 0x7F), so no carry can leak
// between lanes -- unlike the classic subtract-based "hasless" SWAR test,
// whose borrow chains make per-lane counts inexact.
constexpr std::uint64_t lanes_in(std::uint64_t chunk, unsigned first, unsigned last) noexcept {
  auto const ge_first = chunk + ones64() * (0x80 - first);
  auto const ge_last = chunk + ones64() * (0x80 - last);
  return ge_first & ~ge_last & high_bits64();
}

// The number of lanes of an all-ASCII chunk that occupy one terminal cell:
// printable ASCII plus line feed. Everything else below 0x80 is a C0/DEL
// control character and takes no cell (see unicode::is_control).
constexpr std::size_t ascii_cell_lanes(std::uint64_t chunk) noexcept {
  auto const cells = lanes_in(chunk, 0x20, 0x7F) | lanes_in(chunk, 0x0A, 0x0B);
  return std::popcount(cells);
}

// The reference scalar implementation. Constant evaluation cannot run the
// SWAR path (it reads the bytes through memcpy), so glyph_width() keeps this
// for is_constant_evaluated() -- static assertions and constexpr call sites
// get exactly the historical code.
constexpr std::size_t scalar_glyph_width(const char *utf8, std::size_t size) {
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

} // namespace detail

constexpr std::size_t glyph_width(const char *utf8, std::size_t size) {
  if (std::is_constant_evaluated()) {
    return detail::scalar_glyph_width(utf8, size);
  }

  auto width = std::size_t { 0 };
  auto index = std::size_t { 0 };
  while (index < size) {
    // Whole ASCII runs score eight bytes at a time; the lane arithmetic
    // assumes the little-endian lane order the chunk is read in.
    if constexpr (std::endian::native == std::endian::little) {
      while (size - index >= 8) {
        auto chunk = std::uint64_t { };
        std::memcpy(&chunk, utf8 + index, sizeof(chunk));
        if (detail::has_high_bit(chunk)) {
          break; // a multi-byte sequence starts here
        }
        width += detail::ascii_cell_lanes(chunk);
        index += 8;
      }
    }

    // ASCII tail up to the next multi-byte character (also the whole fast
    // path on big-endian targets).
    while (index < size and std::uint8_t(utf8[index]) < 0x80) {
      auto const byte = std::uint8_t(utf8[index]);
      if ((byte >= 0x20 and byte < 0x7F) or byte == 0x0A) {
        width += 1;
      }
      index += 1;
    }
    if (index >= size) {
      break;
    }

    auto cp = char32_t { };
    auto cp_size = mb_to_c32(utf8 + index, size - index, &cp);
    if (cp_size < 0) {
      index += 1;
      continue;
    }
    if (unicode::is_full_width(cp)) {
      width += 2;
    } else if (not (unicode::is_control(cp) or unicode::is_combining(cp))) {
      width += 1;
    }
    index += std::size_t(cp_size);
  }
  return width;
}

constexpr std::size_t glyph_width(const std::string &utf8) {
  return glyph_width(utf8.data(), utf8.size());
}
constexpr std::size_t glyph_width(const std::string_view &utf8) {
  return glyph_width(utf8.data(), utf8.size());
}

// Length of the leading run of ASCII bytes (high bit clear), eight bytes per
// SWAR step. The whole 8-byte check is endian-independent (it only tests the
// high bit of every lane), so it runs everywhere.
inline std::size_t ascii_prefix_length(const char *utf8, std::size_t size) noexcept {
  auto index = std::size_t { 0 };
  while (size - index >= 8) {
    auto chunk = std::uint64_t { };
    std::memcpy(&chunk, utf8 + index, sizeof(chunk));
    if (detail::has_high_bit(chunk)) {
      break;
    }
    index += 8;
  }
  while (index < size and std::uint8_t(utf8[index]) < 0x80) {
    index += 1;
  }
  return index;
}

constexpr std::size_t next_c32(const char *utf8, std::size_t size, std::size_t index, char32_t *cp) {
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

constexpr std::size_t prev_c32(const char *utf8, std::size_t size, std::size_t index, char32_t *cp) {
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

template<typename WChar>
requires (std::is_same_v<WChar, wchar_t> or std::is_same_v<WChar, char16_t>)
constexpr int wc_to_c32(const WChar *ws, const WChar *we, char32_t *c32) {
  if (ws >= we or *ws == 0) {
    return 0;
  }

  // UTF32
  if constexpr (sizeof(WChar) == sizeof(char32_t)) {
    *c32 = *ws;
    return 1;
  } else {
    // UTF16
    auto c0 = ws[0];
    if (c0 < 0xD800 or c0 >= 0xDC00) {
      *c32 = c0;
      return 1;
    } else if ((we - ws) >= 2) {
      auto c1 = ws[1];
      *c32 = ((c0 & 0x3FF) << 10) + (c1 & 0x3FF) + 0x10000;
      return 2;
    } else {
      return -2; // incomplete
    }
  }
}

template<typename WChar>
requires (std::is_same_v<WChar, wchar_t> or std::is_same_v<WChar, char16_t>)
constexpr size_t to_utf8(const WChar *ws, const WChar *we, char *s, char *e) {
  auto *p = s;
  auto wcount = 0U;
  auto cp = char32_t {0}; // code point
  while ((wcount = wc_to_c32(ws, we, &cp)) > 0) {
    p += c32_to_mb(cp, p);
    ws += wcount;
  }
  return p - s;
}

constexpr std::string to_utf8(const wchar_t *data, size_t size) {
  std::string utf8;
  utf8.resize(4 * size);
  auto utf8_size = to_utf8(data, data + size, utf8.data(), utf8.data() + utf8.size());
  utf8.resize(utf8_size);
  return utf8;
}

constexpr std::string to_utf8(const std::wstring &s) {
  return to_utf8(s.data(), s.length());
}

constexpr std::string to_utf8(wchar_t wc) {
  std::string utf8;
  utf8.resize(4);
  auto utf8_size = to_utf8(&wc, &wc + 1, utf8.data(), utf8.data() + utf8.size());
  utf8.resize(utf8_size);
  return utf8;
}

// One UTF-32 code point to UTF-8 (the wide/UTF-16 overloads above are the
// multi-character variants; this is the single-scalar one the text area uses
// when a typed character enters the document). Values above U+10FFFF are not
// representable in UTF-8 and encode to an empty string.
constexpr std::string to_utf8(char32_t code) {
  char bytes[4] = { };
  auto const length = c32_to_mb(code, bytes);
  return std::string(bytes, length);
}

constexpr std::wstring from_utf8(const std::string &s) {
  std::wstring ws;
  ws.reserve(s.length());
  auto index = size_t { 0 };
  while (index < s.size()) {
    auto cp = char32_t { 0 }; // code point
    auto cp_len = mb_to_c32(s.data() + index, s.size() - index, &cp);
    if (cp_len > 0) {
      // UTF32
      if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
        ws += cp;
      } else {
        // UTF16
        if (cp < 0xD800 or (cp > 0xDFFF && cp < 0x10000)) {
          ws += wchar_t(cp);
        } else {
          cp -= 0x010000;
          ws += wchar_t(((cp << 12) >> 22) + 0xD800);
          ws += wchar_t(((cp << 22) >> 22) + 0xDC00);
        }
      }
      index += cp_len;
    } else {
      break;
    }
  }
  return ws;
}

constexpr u8string_view next_token(const u8string &str, size_t &index, char delim) {
  auto from_index = index, to_index = from_index;
  for (; to_index < str.size(); ++to_index) {
    if (str[to_index] == delim) {
      index = to_index + 1;
      return {str.data() + from_index, to_index - from_index};
    }
  }
  index = str.size();
  if (to_index != from_index) {
    return {str.data() + from_index, to_index - from_index};
  } else {
    return {};
  }
}

}
