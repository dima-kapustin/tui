#pragma once

#include <array>
#include <cstdint>
#include <variant>

namespace tui {
namespace detail {
struct Palette16Color;
struct Palette256Color;
struct TrueColor;

constexpr Palette16Color to_palette16(Palette256Color const&);
constexpr TrueColor to_rgb(Palette256Color const&);

struct Palette16Color {
  uint8_t index :4;

  constexpr bool operator==(const Palette16Color &other) const = default;

  constexpr operator Palette256Color() const;
  constexpr operator TrueColor() const;
};

struct Palette256Color {
  uint8_t index;

  constexpr bool operator==(const Palette256Color &other) const = default;

  constexpr operator Palette16Color() const;
  constexpr operator TrueColor() const;
};

using DefaultColor = std::monostate;

struct TrueColor {
  union {
    struct {
      uint32_t blue :8;
      uint32_t green :8;
      uint32_t red :8;
      uint32_t alpha :8;
    };
    uint32_t value;
  };

  constexpr TrueColor() = default;

  constexpr TrueColor(uint32_t value) :
      TrueColor(value >> 16, value >> 8, value >> 0) {
  }

  constexpr TrueColor(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) :
      blue { blue }, green { green }, red { red }, alpha(alpha) {
  }

  constexpr TrueColor(uint8_t red, uint8_t green, uint8_t blue) :
      TrueColor { 255, red, green, blue } {
  }

  constexpr bool operator==(const TrueColor &other) const {
    return this->value == other.value;
  }

  constexpr operator Palette16Color() const;
  constexpr operator Palette256Color() const;
};

//// RGB to XTerm16 conversion algorithm
//
// XTerm16 is actually what's known as a 4-bit RGBI color palette, which
// is not regular. Many solutions out there overlook this fact. Others rely
// on the so-called CIEDE2000 formula, which doesn't convince me either.
//
// The algorithm here consists in converting RGB to HSL as an intermediate
// step. Then we do the following:
//
// * Decide whether the color should be approximated to grayscale or not.
// * If it is grayscale, pick between the 4 levels of gray. Otherwise, pick
//   between the dark and bright color variants. The L component is used for this.
// * If it is color, choose the final color based on the H component.
//
// The result is perceptually closer to the original than the other solutions
// I have seen around. Additionally, this algorithm can be computed in real-time.
//
// This implementation uses integer arithmetic and performs at most one integer
// division.

struct HCL {
  uint8_t h; // [0..HUE_MAX)
  uint8_t c; // [0..255]
  uint8_t l; // [0..255]
};

constexpr uint8_t HUE_PRECISION = 32;
constexpr uint8_t HUE_MAX = 6 * HUE_PRECISION;

constexpr HCL to_hcl(uint8_t r, uint8_t g, uint8_t b) noexcept {
  uint8_t min = std::min(std::min(r, g), b);
  uint8_t max = std::max(std::max(r, g), b);
  uint8_t v = max;
  uint8_t l = uint16_t(max + min) / 2;
  uint8_t c = max - min;
  int16_t h = 0;
  if (c) {
    if (v == r)
      h = int16_t(HUE_PRECISION * (g - b)) / c;
    else if (v == g)
      h = int16_t(HUE_PRECISION * (b - r)) / c + 2 * HUE_PRECISION;
    else if (v == b)
      h = int16_t(HUE_PRECISION * (r - g)) / c + 4 * HUE_PRECISION;

    if (h < 0) {
      h += HUE_MAX;
    } else if (h >= HUE_MAX) {
      h -= HUE_MAX;
    }
  }

  return {uint8_t(h), c, l};
}

constexpr Palette16Color to_palette16(uint8_t red, uint8_t green, uint8_t blue) {
  auto c = to_hcl(red, green, blue);

  auto u8 = [](double d) noexcept {
    return uint8_t(d * 255);
  };

  if (c.c >= 12) { // Color if Chroma >= 12.
    constexpr uint8_t normal[6] = { 0x1, 0x3, 0x2, 0x6, 0x4, 0x5 };
    constexpr uint8_t bright[6] = { 0x9, 0xB, 0xA, 0xE, 0xC, 0xD };
    uint8_t index = uint8_t(c.h < HUE_MAX - HUE_PRECISION / 2 ? c.h + HUE_PRECISION / 2 : c.h - (HUE_MAX - HUE_PRECISION / 2)) / HUE_PRECISION;
    if (c.l < u8(0.5))
      return {normal[index]};
    if (c.l < u8(0.925))
      return {bright[index]};
    return {15};
  } else {
    if (c.l < u8(0.25))
      return {0};
    if (c.l < u8(0.625))
      return {8};
    if (c.l < u8(0.875))
      return {7};
    return {15};
  }
}

constexpr Palette16Color to_palette16(TrueColor const &rgb) {
  return to_palette16(rgb.red, rgb.green, rgb.blue);
}

constexpr Palette16Color to_palette16(Palette256Color const &color) {
  constexpr auto lut = [] {
    std::array<Palette16Color, 256> result { };
    for (auto i = 0U; i < 16U; ++i) {
      result[i] = { uint8_t(i) };
    }

    for (auto i = 0U; i < 6U; ++i) {
      auto r = uint8_t(i ? 55U + i * 40U : 0U);
      for (auto j = 0U; j < 6U; ++j) {
        auto g = uint8_t(j ? 55U + j * 40U : 0U);
        for (auto k = 0U; k < 6U; ++k) {
          auto b = uint8_t(k ? 55U + k * 40U : 0U);
          result[16U + (i * 6U + j) * 6U + k] = to_palette16(r, g, b);
        }
      }
    }
    for (auto i = 0U; i < 24U; ++i) {
      auto l = uint8_t(i * 10U + 8U);
      result[232U + i] = to_palette16(l, l, l);
    }
    return result;
  }();

  return lut[color.index];
}

constexpr Palette256Color to_palette256(TrueColor const &rgb) {
  // The xterm-256color palette consists of:
  //
  // * [0..15]: 16 colors as in xterm-16color.
  // * [16..231]: 216 colors in a 6x6x6 cube.
  // * [232..255]: 24 grayscale colors.
  //
  // This function does not return indices in the range [0..15]. For that,
  // use 'to_palette16(TrueColor)' instead.
  //
  // Dark colors are underrepresented in the 6x6x6 cube. The channel values
  // [0, 1, 2, 3, 4, 5] correspond to the 8-bit values
  // [0, 95, 135, 175, 215, 255]. Thus there is a distance of 40 between
  // values, except for 0. Any 8-bit value smaller than 95 - 40/2 = 75
  // would have to be mapped into 0. To compensate a bit for this, we allow
  // values [55..74] to also be mapped into 1.
  //
  // Additionally, we fall back on the grayscale colors whenever using
  // the 6x6x6 color cube would round the color to pure black. This
  // makes it possible to preserve details that would otherwise be lost.
  auto to_color = [](TrueColor const &c) -> Palette256Color {
    auto scale = [](uint8_t c) {
      c += 20 & -(c < 75);
      return uint8_t(std::max(c, uint8_t(35)) - 35U) / 40U;
    };
    auto r = scale(c.red), g = scale(c.green), b = scale(c.blue);
    return {uint8_t(16U + (r * 6U + g) * 6U + b)};
  };
  auto to_grayscale = [](uint8_t l) -> Palette256Color {
    if (l < 8 - 5)
      return {16U};
    if (l >= 238 + 5)
      return {231U};
    return {uint8_t(232U + (std::max(l, uint8_t(3)) - 3U) / 10U)};
  };

  auto result = to_color(rgb);
  if (rgb != to_rgb(result)) {
    auto min = std::min(std::min(rgb.red, rgb.green), rgb.blue), max = std::max(std::max(rgb.red, rgb.green), rgb.blue);
    auto C = max - min; // Chroma in the HSL/HSV theory.
    if (C < 12 or result.index == 16) { // Grayscale if Chroma < 12 or rounded to black.
      uint8_t L = unsigned(max + min) / 2; // Lightness, as in HSL.
      return to_grayscale(L);
    }
  }
  return result;
}

constexpr TrueColor to_rgb(Palette256Color const &color) {
  constexpr auto lut = [] {
    // Indices 16..255 only.
    std::array<TrueColor, 256> result { };
    for (auto i = 0U; i < 6U; ++i) {
      auto r = uint8_t(i ? 55U + i * 40U : 0U);
      for (auto j = 0U; j < 6U; ++j) {
        auto g = uint8_t(j ? 55U + j * 40U : 0U);
        for (auto k = 0U; k < 6U; ++k) {
          auto b = uint8_t(k ? 55U + k * 40U : 0U);
          result[16U + (i * 6U + j) * 6U + k] = { r, g, b };
        }
      }
    }
    for (auto i = 0U; i < 24U; ++i) {
      auto l = uint8_t(i * 10U + 8U);
      result[232U + i] = { l, l, l };
    }
    return result;
  }();

  return lut[color.index];
}

constexpr TrueColor complementary(TrueColor const &rgb) {
  return {255U - rgb.red, 255U - rgb.green, 255U - rgb.blue};
}

}

using TerminalColor = std::variant<detail::DefaultColor, detail::Palette16Color, detail::Palette256Color, detail::TrueColor>;

/*@formatter:off*/
constexpr TerminalColor BLACK              = detail::Palette16Color { 0 };
constexpr TerminalColor RED                = detail::Palette16Color { 1 };
constexpr TerminalColor GREEN              = detail::Palette16Color { 2 };
constexpr TerminalColor YELLOW             = detail::Palette16Color { 3 };
constexpr TerminalColor BLUE               = detail::Palette16Color { 4 };
constexpr TerminalColor MAGENTA            = detail::Palette16Color { 5 };
constexpr TerminalColor CYAN               = detail::Palette16Color { 6 };
constexpr TerminalColor GRAYLIGHT          = detail::Palette16Color { 7 };
constexpr TerminalColor GRAYDARK           = detail::Palette16Color { 8 };
constexpr TerminalColor REDLIGHT           = detail::Palette16Color { 9 };
constexpr TerminalColor GREENLIGHT         = detail::Palette16Color { 10 };
constexpr TerminalColor YELLOWLIGHT        = detail::Palette16Color { 11 };
constexpr TerminalColor BLUELIGHT          = detail::Palette16Color { 12 };
constexpr TerminalColor MAGENTALIGHT       = detail::Palette16Color { 13 };
constexpr TerminalColor CYANLIGHT          = detail::Palette16Color { 14 };
constexpr TerminalColor WHITE              = detail::Palette16Color { 15 };

constexpr TerminalColor AQUAMARINE1        = detail::Palette256Color { 122 };
constexpr TerminalColor AQUAMARINE1BIS     = detail::Palette256Color { 86 };
constexpr TerminalColor AQUAMARINE3        = detail::Palette256Color { 79 };
constexpr TerminalColor BLUE1              = detail::Palette256Color { 21 };
constexpr TerminalColor BLUE3              = detail::Palette256Color { 19 };
constexpr TerminalColor BLUE3BIS           = detail::Palette256Color { 20 };
constexpr TerminalColor BLUEVIOLET         = detail::Palette256Color { 57 };
constexpr TerminalColor CADETBLUE          = detail::Palette256Color { 72 };
constexpr TerminalColor CADETBLUEBIS       = detail::Palette256Color { 73 };
constexpr TerminalColor CHARTREUSE1        = detail::Palette256Color { 118 };
constexpr TerminalColor CHARTREUSE2        = detail::Palette256Color { 112 };
constexpr TerminalColor CHARTREUSE2BIS     = detail::Palette256Color { 82 };
constexpr TerminalColor CHARTREUSE3        = detail::Palette256Color { 70 };
constexpr TerminalColor CHARTREUSE3BIS     = detail::Palette256Color { 76 };
constexpr TerminalColor CHARTREUSE4        = detail::Palette256Color { 64 };
constexpr TerminalColor CORNFLOWERBLUE     = detail::Palette256Color { 69 };
constexpr TerminalColor CORNSILK1          = detail::Palette256Color { 230 };
constexpr TerminalColor CYAN1              = detail::Palette256Color { 51 };
constexpr TerminalColor CYAN2              = detail::Palette256Color { 50 };
constexpr TerminalColor CYAN3              = detail::Palette256Color { 43 };
constexpr TerminalColor DARKBLUE           = detail::Palette256Color { 18 };
constexpr TerminalColor DARKCYAN           = detail::Palette256Color { 36 };
constexpr TerminalColor DARKGOLDENROD      = detail::Palette256Color { 136 };
constexpr TerminalColor DARKGREEN          = detail::Palette256Color { 22 };
constexpr TerminalColor DARKKHAKI          = detail::Palette256Color { 143 };
constexpr TerminalColor DARKMAGENTA        = detail::Palette256Color { 90 };
constexpr TerminalColor DARKMAGENTABIS     = detail::Palette256Color { 91 };
constexpr TerminalColor DARKOLIVEGREEN1    = detail::Palette256Color { 191 };
constexpr TerminalColor DARKOLIVEGREEN1BIS = detail::Palette256Color { 192 };
constexpr TerminalColor DARKOLIVEGREEN2    = detail::Palette256Color { 155 };
constexpr TerminalColor DARKOLIVEGREEN3    = detail::Palette256Color { 107 };
constexpr TerminalColor DARKOLIVEGREEN3BIS = detail::Palette256Color { 113 };
constexpr TerminalColor DARKOLIVEGREEN3TER = detail::Palette256Color { 149 };
constexpr TerminalColor DARKORANGE         = detail::Palette256Color { 208 };
constexpr TerminalColor DARKORANGE3        = detail::Palette256Color { 130 };
constexpr TerminalColor DARKORANGE3BIS     = detail::Palette256Color { 166 };
constexpr TerminalColor DARKRED            = detail::Palette256Color { 52 };
constexpr TerminalColor DARKREDBIS         = detail::Palette256Color { 88 };
constexpr TerminalColor DARKSEAGREEN       = detail::Palette256Color { 108 };
constexpr TerminalColor DARKSEAGREEN1      = detail::Palette256Color { 158 };
constexpr TerminalColor DARKSEAGREEN1BIS   = detail::Palette256Color { 193 };
constexpr TerminalColor DARKSEAGREEN2      = detail::Palette256Color { 151 };
constexpr TerminalColor DARKSEAGREEN2BIS   = detail::Palette256Color { 157 };
constexpr TerminalColor DARKSEAGREEN3      = detail::Palette256Color { 115 };
constexpr TerminalColor DARKSEAGREEN3BIS   = detail::Palette256Color { 150 };
constexpr TerminalColor DARKSEAGREEN4      = detail::Palette256Color { 65 };
constexpr TerminalColor DARKSEAGREEN4BIS   = detail::Palette256Color { 71 };
constexpr TerminalColor DARKSLATEGRAY1     = detail::Palette256Color { 123 };
constexpr TerminalColor DARKSLATEGRAY2     = detail::Palette256Color { 87 };
constexpr TerminalColor DARKSLATEGRAY3     = detail::Palette256Color { 116 };
constexpr TerminalColor DARKTURQUOISE      = detail::Palette256Color { 44 };
constexpr TerminalColor DARKVIOLET         = detail::Palette256Color { 128 };
constexpr TerminalColor DARKVIOLETBIS      = detail::Palette256Color { 92 };
constexpr TerminalColor DEEPPINK1          = detail::Palette256Color { 198 };
constexpr TerminalColor DEEPPINK1BIS       = detail::Palette256Color { 199 };
constexpr TerminalColor DEEPPINK2          = detail::Palette256Color { 197 };
constexpr TerminalColor DEEPPINK3          = detail::Palette256Color { 161 };
constexpr TerminalColor DEEPPINK3BIS       = detail::Palette256Color { 162 };
constexpr TerminalColor DEEPPINK4          = detail::Palette256Color { 125 };
constexpr TerminalColor DEEPPINK4BIS       = detail::Palette256Color { 89 };
constexpr TerminalColor DEEPPINK4TER       = detail::Palette256Color { 53 };
constexpr TerminalColor DEEPSKYBLUE1       = detail::Palette256Color { 39 };
constexpr TerminalColor DEEPSKYBLUE2       = detail::Palette256Color { 38 };
constexpr TerminalColor DEEPSKYBLUE3       = detail::Palette256Color { 31 };
constexpr TerminalColor DEEPSKYBLUE3BIS    = detail::Palette256Color { 32 };
constexpr TerminalColor DEEPSKYBLUE4       = detail::Palette256Color { 23 };
constexpr TerminalColor DEEPSKYBLUE4BIS    = detail::Palette256Color { 24 };
constexpr TerminalColor DEEPSKYBLUE4TER    = detail::Palette256Color { 25 };
constexpr TerminalColor DODGERBLUE1        = detail::Palette256Color { 33 };
constexpr TerminalColor DODGERBLUE2        = detail::Palette256Color { 27 };
constexpr TerminalColor DODGERBLUE3        = detail::Palette256Color { 26 };
constexpr TerminalColor GOLD1              = detail::Palette256Color { 220 };
constexpr TerminalColor GOLD3              = detail::Palette256Color { 142 };
constexpr TerminalColor GOLD3BIS           = detail::Palette256Color { 178 };
constexpr TerminalColor GREEN1             = detail::Palette256Color { 46 };
constexpr TerminalColor GREEN3             = detail::Palette256Color { 34 };
constexpr TerminalColor GREEN3BIS          = detail::Palette256Color { 40 };
constexpr TerminalColor GREEN4             = detail::Palette256Color { 28 };
constexpr TerminalColor GREENYELLOW        = detail::Palette256Color { 154 };
constexpr TerminalColor GREY0              = detail::Palette256Color { 16 };
constexpr TerminalColor GREY100            = detail::Palette256Color { 231 };
constexpr TerminalColor GREY11             = detail::Palette256Color { 234 };
constexpr TerminalColor GREY15             = detail::Palette256Color { 235 };
constexpr TerminalColor GREY19             = detail::Palette256Color { 236 };
constexpr TerminalColor GREY23             = detail::Palette256Color { 237 };
constexpr TerminalColor GREY27             = detail::Palette256Color { 238 };
constexpr TerminalColor GREY3              = detail::Palette256Color { 232 };
constexpr TerminalColor GREY30             = detail::Palette256Color { 239 };
constexpr TerminalColor GREY35             = detail::Palette256Color { 240 };
constexpr TerminalColor GREY37             = detail::Palette256Color { 59 };
constexpr TerminalColor GREY39             = detail::Palette256Color { 241 };
constexpr TerminalColor GREY42             = detail::Palette256Color { 242 };
constexpr TerminalColor GREY46             = detail::Palette256Color { 243 };
constexpr TerminalColor GREY50             = detail::Palette256Color { 244 };
constexpr TerminalColor GREY53             = detail::Palette256Color { 102 };
constexpr TerminalColor GREY54             = detail::Palette256Color { 245 };
constexpr TerminalColor GREY58             = detail::Palette256Color { 246 };
constexpr TerminalColor GREY62             = detail::Palette256Color { 247 };
constexpr TerminalColor GREY63             = detail::Palette256Color { 139 };
constexpr TerminalColor GREY66             = detail::Palette256Color { 248 };
constexpr TerminalColor GREY69             = detail::Palette256Color { 145 };
constexpr TerminalColor GREY7              = detail::Palette256Color { 233 };
constexpr TerminalColor GREY70             = detail::Palette256Color { 249 };
constexpr TerminalColor GREY74             = detail::Palette256Color { 250 };
constexpr TerminalColor GREY78             = detail::Palette256Color { 251 };
constexpr TerminalColor GREY82             = detail::Palette256Color { 252 };
constexpr TerminalColor GREY84             = detail::Palette256Color { 188 };
constexpr TerminalColor GREY85             = detail::Palette256Color { 253 };
constexpr TerminalColor GREY89             = detail::Palette256Color { 254 };
constexpr TerminalColor GREY93             = detail::Palette256Color { 255 };
constexpr TerminalColor HONEYDEW2          = detail::Palette256Color { 194 };
constexpr TerminalColor HOTPINK            = detail::Palette256Color { 205 };
constexpr TerminalColor HOTPINK2           = detail::Palette256Color { 169 };
constexpr TerminalColor HOTPINK3           = detail::Palette256Color { 132 };
constexpr TerminalColor HOTPINK3BIS        = detail::Palette256Color { 168 };
constexpr TerminalColor HOTPINKBIS         = detail::Palette256Color { 206 };
constexpr TerminalColor INDIANRED          = detail::Palette256Color { 131 };
constexpr TerminalColor INDIANRED1         = detail::Palette256Color { 203 };
constexpr TerminalColor INDIANRED1BIS      = detail::Palette256Color { 204 };
constexpr TerminalColor INDIANREDBIS       = detail::Palette256Color { 167 };
constexpr TerminalColor KHAKI1             = detail::Palette256Color { 228 };
constexpr TerminalColor KHAKI3             = detail::Palette256Color { 185 };
constexpr TerminalColor LIGHTCORAL         = detail::Palette256Color { 210 };
constexpr TerminalColor LIGHTCYAN1BIS      = detail::Palette256Color { 195 };
constexpr TerminalColor LIGHTCYAN3         = detail::Palette256Color { 152 };
constexpr TerminalColor LIGHTGOLDENROD1    = detail::Palette256Color { 227 };
constexpr TerminalColor LIGHTGOLDENROD2    = detail::Palette256Color { 186 };
constexpr TerminalColor LIGHTGOLDENROD2BIS = detail::Palette256Color { 221 };
constexpr TerminalColor LIGHTGOLDENROD2TER = detail::Palette256Color { 222 };
constexpr TerminalColor LIGHTGOLDENROD3    = detail::Palette256Color { 179 };
constexpr TerminalColor LIGHTGREEN         = detail::Palette256Color { 119 };
constexpr TerminalColor LIGHTGREENBIS      = detail::Palette256Color { 120 };
constexpr TerminalColor LIGHTPINK1         = detail::Palette256Color { 217 };
constexpr TerminalColor LIGHTPINK3         = detail::Palette256Color { 174 };
constexpr TerminalColor LIGHTPINK4         = detail::Palette256Color { 95 };
constexpr TerminalColor LIGHTSALMON1       = detail::Palette256Color { 216 };
constexpr TerminalColor LIGHTSALMON3       = detail::Palette256Color { 137 };
constexpr TerminalColor LIGHTSALMON3BIS    = detail::Palette256Color { 173 };
constexpr TerminalColor LIGHTSEAGREEN      = detail::Palette256Color { 37 };
constexpr TerminalColor LIGHTSKYBLUE1      = detail::Palette256Color { 153 };
constexpr TerminalColor LIGHTSKYBLUE3      = detail::Palette256Color { 109 };
constexpr TerminalColor LIGHTSKYBLUE3BIS   = detail::Palette256Color { 110 };
constexpr TerminalColor LIGHTSLATEBLUE     = detail::Palette256Color { 105 };
constexpr TerminalColor LIGHTSLATEGREY     = detail::Palette256Color { 103 };
constexpr TerminalColor LIGHTSTEELBLUE     = detail::Palette256Color { 147 };
constexpr TerminalColor LIGHTSTEELBLUE1    = detail::Palette256Color { 189 };
constexpr TerminalColor LIGHTSTEELBLUE3    = detail::Palette256Color { 146 };
constexpr TerminalColor LIGHTYELLOW3       = detail::Palette256Color { 187 };
constexpr TerminalColor MAGENTA1           = detail::Palette256Color { 201 };
constexpr TerminalColor MAGENTA2           = detail::Palette256Color { 165 };
constexpr TerminalColor MAGENTA2BIS        = detail::Palette256Color { 200 };
constexpr TerminalColor MAGENTA3           = detail::Palette256Color { 127 };
constexpr TerminalColor MAGENTA3BIS        = detail::Palette256Color { 163 };
constexpr TerminalColor MAGENTA3TER        = detail::Palette256Color { 164 };
constexpr TerminalColor MEDIUMORCHID       = detail::Palette256Color { 134 };
constexpr TerminalColor MEDIUMORCHID1      = detail::Palette256Color { 171 };
constexpr TerminalColor MEDIUMORCHID1BIS   = detail::Palette256Color { 207 };
constexpr TerminalColor MEDIUMORCHID3      = detail::Palette256Color { 133 };
constexpr TerminalColor MEDIUMPURPLE       = detail::Palette256Color { 104 };
constexpr TerminalColor MEDIUMPURPLE1      = detail::Palette256Color { 141 };
constexpr TerminalColor MEDIUMPURPLE2      = detail::Palette256Color { 135 };
constexpr TerminalColor MEDIUMPURPLE2BIS   = detail::Palette256Color { 140 };
constexpr TerminalColor MEDIUMPURPLE3      = detail::Palette256Color { 97 };
constexpr TerminalColor MEDIUMPURPLE3BIS   = detail::Palette256Color { 98 };
constexpr TerminalColor MEDIUMPURPLE4      = detail::Palette256Color { 60 };
constexpr TerminalColor MEDIUMSPRINGGREEN  = detail::Palette256Color { 49 };
constexpr TerminalColor MEDIUMTURQUOISE    = detail::Palette256Color { 80 };
constexpr TerminalColor MEDIUMVIOLETRED    = detail::Palette256Color { 126 };
constexpr TerminalColor MISTYROSE1         = detail::Palette256Color { 224 };
constexpr TerminalColor MISTYROSE3         = detail::Palette256Color { 181 };
constexpr TerminalColor NAVAJOWHITE1       = detail::Palette256Color { 223 };
constexpr TerminalColor NAVAJOWHITE3       = detail::Palette256Color { 144 };
constexpr TerminalColor NAVYBLUE           = detail::Palette256Color { 17 };
constexpr TerminalColor ORANGE1            = detail::Palette256Color { 214 };
constexpr TerminalColor ORANGE3            = detail::Palette256Color { 172 };
constexpr TerminalColor ORANGE4            = detail::Palette256Color { 58 };
constexpr TerminalColor ORANGE4BIS         = detail::Palette256Color { 94 };
constexpr TerminalColor ORANGERED1         = detail::Palette256Color { 202 };
constexpr TerminalColor ORCHID             = detail::Palette256Color { 170 };
constexpr TerminalColor ORCHID1            = detail::Palette256Color { 213 };
constexpr TerminalColor ORCHID2            = detail::Palette256Color { 212 };
constexpr TerminalColor PALEGREEN1         = detail::Palette256Color { 121 };
constexpr TerminalColor PALEGREEN1BIS      = detail::Palette256Color { 156 };
constexpr TerminalColor PALEGREEN3         = detail::Palette256Color { 114 };
constexpr TerminalColor PALEGREEN3BIS      = detail::Palette256Color { 77 };
constexpr TerminalColor PALETURQUOISE1     = detail::Palette256Color { 159 };
constexpr TerminalColor PALETURQUOISE4     = detail::Palette256Color { 66 };
constexpr TerminalColor PALEVIOLETRED1     = detail::Palette256Color { 211 };
constexpr TerminalColor PINK1              = detail::Palette256Color { 218 };
constexpr TerminalColor PINK3              = detail::Palette256Color { 175 };
constexpr TerminalColor PLUM1              = detail::Palette256Color { 219 };
constexpr TerminalColor PLUM2              = detail::Palette256Color { 183 };
constexpr TerminalColor PLUM3              = detail::Palette256Color { 176 };
constexpr TerminalColor PLUM4              = detail::Palette256Color { 96 };
constexpr TerminalColor PURPLE             = detail::Palette256Color { 129 };
constexpr TerminalColor PURPLE3            = detail::Palette256Color { 56 };
constexpr TerminalColor PURPLE4            = detail::Palette256Color { 54 };
constexpr TerminalColor PURPLE4BIS         = detail::Palette256Color { 55 };
constexpr TerminalColor PURPLEBIS          = detail::Palette256Color { 93 };
constexpr TerminalColor RED1               = detail::Palette256Color { 196 };
constexpr TerminalColor RED3               = detail::Palette256Color { 124 };
constexpr TerminalColor RED3BIS            = detail::Palette256Color { 160 };
constexpr TerminalColor ROSYBROWN          = detail::Palette256Color { 138 };
constexpr TerminalColor ROYALBLUE1         = detail::Palette256Color { 63 };
constexpr TerminalColor SALMON1            = detail::Palette256Color { 209 };
constexpr TerminalColor SANDYBROWN         = detail::Palette256Color { 215 };
constexpr TerminalColor SEAGREEN1          = detail::Palette256Color { 84 };
constexpr TerminalColor SEAGREEN1BIS       = detail::Palette256Color { 85 };
constexpr TerminalColor SEAGREEN2          = detail::Palette256Color { 83 };
constexpr TerminalColor SEAGREEN3          = detail::Palette256Color { 78 };
constexpr TerminalColor SKYBLUE1           = detail::Palette256Color { 117 };
constexpr TerminalColor SKYBLUE2           = detail::Palette256Color { 111 };
constexpr TerminalColor SKYBLUE3           = detail::Palette256Color { 74 };
constexpr TerminalColor SLATEBLUE1         = detail::Palette256Color { 99 };
constexpr TerminalColor SLATEBLUE3         = detail::Palette256Color { 61 };
constexpr TerminalColor SLATEBLUE3BIS      = detail::Palette256Color { 62 };
constexpr TerminalColor SPRINGGREEN1       = detail::Palette256Color { 48 };
constexpr TerminalColor SPRINGGREEN2       = detail::Palette256Color { 42 };
constexpr TerminalColor SPRINGGREEN2BIS    = detail::Palette256Color { 47 };
constexpr TerminalColor SPRINGGREEN3       = detail::Palette256Color { 35 };
constexpr TerminalColor SPRINGGREEN3BIS    = detail::Palette256Color { 41 };
constexpr TerminalColor SPRINGGREEN4       = detail::Palette256Color { 29 };
constexpr TerminalColor STEELBLUE          = detail::Palette256Color { 67 };
constexpr TerminalColor STEELBLUE1         = detail::Palette256Color { 75 };
constexpr TerminalColor STEELBLUE1BIS      = detail::Palette256Color { 81 };
constexpr TerminalColor STEELBLUE3         = detail::Palette256Color { 68 };
constexpr TerminalColor TAN                = detail::Palette256Color { 180 };
constexpr TerminalColor THISTLE1           = detail::Palette256Color { 225 };
constexpr TerminalColor THISTLE3           = detail::Palette256Color { 182 };
constexpr TerminalColor TURQUOISE2         = detail::Palette256Color { 45 };
constexpr TerminalColor TURQUOISE4         = detail::Palette256Color { 30 };
constexpr TerminalColor VIOLET             = detail::Palette256Color { 177 };
constexpr TerminalColor WHEAT1             = detail::Palette256Color { 229 };
constexpr TerminalColor WHEAT4             = detail::Palette256Color { 101 };
constexpr TerminalColor YELLOW1            = detail::Palette256Color { 226 };
constexpr TerminalColor YELLOW2            = detail::Palette256Color { 190 };
constexpr TerminalColor YELLOW3            = detail::Palette256Color { 148 };
constexpr TerminalColor YELLOW3BIS         = detail::Palette256Color { 184 };
constexpr TerminalColor YELLOW4            = detail::Palette256Color { 100 };
constexpr TerminalColor YELLOW4BIS         = detail::Palette256Color { 106 };
/*@formatter:on*/

}
