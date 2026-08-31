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
constexpr Palette16Color to_palette16(TrueColor const&);
constexpr Palette256Color to_palette256(TrueColor const&);
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
      uint8_t blue;
      uint8_t green;
      uint8_t red;
      uint8_t alpha;
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

static_assert(sizeof(TrueColor) == 4);

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
    std::array<TrueColor, 256> result { };
    result[0] = { 0, 0, 0 }; // Black
    result[1] = { 128, 0, 0 }; // Red
    result[2] = { 0, 128, 0 }; // Green
    result[3] = { 128, 128, 0 }; // Yellow
    result[4] = { 0, 0, 128 }; // Blue
    result[5] = { 128, 0, 128 }; // Magenta
    result[6] = { 0, 128, 128 }; // Cyan
    result[7] = { 192, 192, 192 }; // GrayLight
    result[8] = { 128, 128, 128 }; // GrayDark
    result[9] = { 255, 0, 0 }; // RedLight
    result[10] = { 0, 255, 0 }; // GreenLight
    result[11] = { 255, 255, 0 }; // YellowLight
    result[12] = { 0, 0, 255 }; // BlueLight
    result[13] = { 255, 0, 255 }; // MagentaLight
    result[14] = { 0, 255, 255 }; // CyanLight
    result[15] = { 255, 255, 255 }; // White

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

constexpr Palette16Color::operator Palette256Color() const {
  return {this->index};
}

constexpr Palette16Color::operator TrueColor() const {
  return to_rgb(*this);
}

constexpr Palette256Color::operator Palette16Color() const {
  return to_palette16(*this);
}

constexpr Palette256Color::operator TrueColor() const {
  return to_rgb(*this);
}

constexpr TrueColor::operator Palette16Color() const {
  return to_palette16(*this);
}

constexpr TrueColor::operator Palette256Color() const {
  return to_palette256(*this);
}

constexpr TrueColor complementary(TrueColor const &rgb) {
  return {uint8_t(255U - rgb.red), uint8_t(255U - rgb.green), uint8_t(255U - rgb.blue)};
}

}

using TextColor = std::variant<detail::DefaultColor, detail::Palette16Color, detail::Palette256Color, detail::TrueColor>;

/*@formatter:off*/
constexpr TextColor BLACK                = detail::Palette16Color { 0 };
constexpr TextColor RED                  = detail::Palette16Color { 1 };
constexpr TextColor GREEN                = detail::Palette16Color { 2 };
constexpr TextColor YELLOW               = detail::Palette16Color { 3 };
constexpr TextColor BLUE                 = detail::Palette16Color { 4 };
constexpr TextColor MAGENTA              = detail::Palette16Color { 5 };
constexpr TextColor CYAN                 = detail::Palette16Color { 6 };
constexpr TextColor GRAY_LIGHT           = detail::Palette16Color { 7 };
constexpr TextColor GRAY_DARK            = detail::Palette16Color { 8 };
constexpr TextColor RED_LIGHT            = detail::Palette16Color { 9 };
constexpr TextColor GREEN_LIGHT          = detail::Palette16Color { 10 };
constexpr TextColor YELLOW_LIGHT         = detail::Palette16Color { 11 };
constexpr TextColor BLUE_LIGHT           = detail::Palette16Color { 12 };
constexpr TextColor MAGENTA_LIGHT        = detail::Palette16Color { 13 };
constexpr TextColor CYAN_LIGHT           = detail::Palette16Color { 14 };
constexpr TextColor WHITE                = detail::Palette16Color { 15 };

constexpr TextColor AQUAMARINE1          = detail::Palette256Color { 122 };
constexpr TextColor AQUAMARINE1BIS       = detail::Palette256Color { 86 };
constexpr TextColor AQUAMARINE3          = detail::Palette256Color { 79 };
constexpr TextColor BLUE1                = detail::Palette256Color { 21 };
constexpr TextColor BLUE3                = detail::Palette256Color { 19 };
constexpr TextColor BLUE3BIS             = detail::Palette256Color { 20 };
constexpr TextColor BLUE_VIOLET          = detail::Palette256Color { 57 };
constexpr TextColor CADET_BLUE           = detail::Palette256Color { 72 };
constexpr TextColor CADET_BLUEBIS        = detail::Palette256Color { 73 };
constexpr TextColor CHART_REUSE1         = detail::Palette256Color { 118 };
constexpr TextColor CHART_REUSE2         = detail::Palette256Color { 112 };
constexpr TextColor CHART_REUSE2BIS      = detail::Palette256Color { 82 };
constexpr TextColor CHART_REUSE3         = detail::Palette256Color { 70 };
constexpr TextColor CHART_REUSE3BIS      = detail::Palette256Color { 76 };
constexpr TextColor CHART_REUSE4         = detail::Palette256Color { 64 };
constexpr TextColor CORNFLOWER_BLUE      = detail::Palette256Color { 69 };
constexpr TextColor CORNSILK1            = detail::Palette256Color { 230 };
constexpr TextColor CYAN1                = detail::Palette256Color { 51 };
constexpr TextColor CYAN2                = detail::Palette256Color { 50 };
constexpr TextColor CYAN3                = detail::Palette256Color { 43 };
constexpr TextColor DARK_BLUE            = detail::Palette256Color { 18 };
constexpr TextColor DARK_CYAN            = detail::Palette256Color { 36 };
constexpr TextColor DARK_GOLDENROD       = detail::Palette256Color { 136 };
constexpr TextColor DARK_GREEN           = detail::Palette256Color { 22 };
constexpr TextColor DARK_KHAKI           = detail::Palette256Color { 143 };
constexpr TextColor DARK_MAGENTA         = detail::Palette256Color { 90 };
constexpr TextColor DARK_MAGENTABIS      = detail::Palette256Color { 91 };
constexpr TextColor DARK_OLIVE_GREEN1    = detail::Palette256Color { 191 };
constexpr TextColor DARK_OLIVE_GREEN1BIS = detail::Palette256Color { 192 };
constexpr TextColor DARK_OLIVE_GREEN2    = detail::Palette256Color { 155 };
constexpr TextColor DARK_OLIVE_GREEN3    = detail::Palette256Color { 107 };
constexpr TextColor DARK_OLIVE_GREEN3BIS = detail::Palette256Color { 113 };
constexpr TextColor DARK_OLIVE_GREEN3TER = detail::Palette256Color { 149 };
constexpr TextColor DARK_ORANGE          = detail::Palette256Color { 208 };
constexpr TextColor DARK_ORANGE3         = detail::Palette256Color { 130 };
constexpr TextColor DARK_ORANGE3BIS      = detail::Palette256Color { 166 };
constexpr TextColor DARK_RED             = detail::Palette256Color { 52 };
constexpr TextColor DARK_REDBIS          = detail::Palette256Color { 88 };
constexpr TextColor DARK_SEAGREEN        = detail::Palette256Color { 108 };
constexpr TextColor DARK_SEAGREEN1       = detail::Palette256Color { 158 };
constexpr TextColor DARK_SEAGREEN1BIS    = detail::Palette256Color { 193 };
constexpr TextColor DARK_SEAGREEN2       = detail::Palette256Color { 151 };
constexpr TextColor DARK_SEAGREEN2BIS    = detail::Palette256Color { 157 };
constexpr TextColor DARK_SEAGREEN3       = detail::Palette256Color { 115 };
constexpr TextColor DARK_SEAGREEN3BIS    = detail::Palette256Color { 150 };
constexpr TextColor DARK_SEAGREEN4       = detail::Palette256Color { 65 };
constexpr TextColor DARK_SEAGREEN4BIS    = detail::Palette256Color { 71 };
constexpr TextColor DARK_SLATEGRAY1      = detail::Palette256Color { 123 };
constexpr TextColor DARK_SLATEGRAY2      = detail::Palette256Color { 87 };
constexpr TextColor DARK_SLATEGRAY3      = detail::Palette256Color { 116 };
constexpr TextColor DARK_TURQUOISE       = detail::Palette256Color { 44 };
constexpr TextColor DARK_VIOLET          = detail::Palette256Color { 128 };
constexpr TextColor DARK_VIOLETBIS       = detail::Palette256Color { 92 };
constexpr TextColor DEEP_PINK1           = detail::Palette256Color { 198 };
constexpr TextColor DEEP_PINK1BIS        = detail::Palette256Color { 199 };
constexpr TextColor DEEP_PINK2           = detail::Palette256Color { 197 };
constexpr TextColor DEEP_PINK3           = detail::Palette256Color { 161 };
constexpr TextColor DEEP_PINK3BIS        = detail::Palette256Color { 162 };
constexpr TextColor DEEP_PINK4           = detail::Palette256Color { 125 };
constexpr TextColor DEEP_PINK4BIS        = detail::Palette256Color { 89 };
constexpr TextColor DEEP_PINK4TER        = detail::Palette256Color { 53 };
constexpr TextColor DEEP_SKYBLUE1        = detail::Palette256Color { 39 };
constexpr TextColor DEEP_SKYBLUE2        = detail::Palette256Color { 38 };
constexpr TextColor DEEP_SKYBLUE3        = detail::Palette256Color { 31 };
constexpr TextColor DEEP_SKYBLUE3BIS     = detail::Palette256Color { 32 };
constexpr TextColor DEEP_SKYBLUE4        = detail::Palette256Color { 23 };
constexpr TextColor DEEP_SKYBLUE4BIS     = detail::Palette256Color { 24 };
constexpr TextColor DEEP_SKYBLUE4TER     = detail::Palette256Color { 25 };
constexpr TextColor DODGER_BLUE1         = detail::Palette256Color { 33 };
constexpr TextColor DODGER_BLUE2         = detail::Palette256Color { 27 };
constexpr TextColor DODGER_BLUE3         = detail::Palette256Color { 26 };
constexpr TextColor GOLD1                = detail::Palette256Color { 220 };
constexpr TextColor GOLD3                = detail::Palette256Color { 142 };
constexpr TextColor GOLD3BIS             = detail::Palette256Color { 178 };
constexpr TextColor GREEN1               = detail::Palette256Color { 46 };
constexpr TextColor GREEN3               = detail::Palette256Color { 34 };
constexpr TextColor GREEN3BIS            = detail::Palette256Color { 40 };
constexpr TextColor GREEN4               = detail::Palette256Color { 28 };
constexpr TextColor GREEN_YELLOW         = detail::Palette256Color { 154 };
constexpr TextColor GREY0                = detail::Palette256Color { 16 };
constexpr TextColor GREY100              = detail::Palette256Color { 231 };
constexpr TextColor GREY11               = detail::Palette256Color { 234 };
constexpr TextColor GREY15               = detail::Palette256Color { 235 };
constexpr TextColor GREY19               = detail::Palette256Color { 236 };
constexpr TextColor GREY23               = detail::Palette256Color { 237 };
constexpr TextColor GREY27               = detail::Palette256Color { 238 };
constexpr TextColor GREY3                = detail::Palette256Color { 232 };
constexpr TextColor GREY30               = detail::Palette256Color { 239 };
constexpr TextColor GREY35               = detail::Palette256Color { 240 };
constexpr TextColor GREY37               = detail::Palette256Color { 59 };
constexpr TextColor GREY39               = detail::Palette256Color { 241 };
constexpr TextColor GREY42               = detail::Palette256Color { 242 };
constexpr TextColor GREY46               = detail::Palette256Color { 243 };
constexpr TextColor GREY50               = detail::Palette256Color { 244 };
constexpr TextColor GREY53               = detail::Palette256Color { 102 };
constexpr TextColor GREY54               = detail::Palette256Color { 245 };
constexpr TextColor GREY58               = detail::Palette256Color { 246 };
constexpr TextColor GREY62               = detail::Palette256Color { 247 };
constexpr TextColor GREY63               = detail::Palette256Color { 139 };
constexpr TextColor GREY66               = detail::Palette256Color { 248 };
constexpr TextColor GREY69               = detail::Palette256Color { 145 };
constexpr TextColor GREY7                = detail::Palette256Color { 233 };
constexpr TextColor GREY70               = detail::Palette256Color { 249 };
constexpr TextColor GREY74               = detail::Palette256Color { 250 };
constexpr TextColor GREY78               = detail::Palette256Color { 251 };
constexpr TextColor GREY82               = detail::Palette256Color { 252 };
constexpr TextColor GREY84               = detail::Palette256Color { 188 };
constexpr TextColor GREY85               = detail::Palette256Color { 253 };
constexpr TextColor GREY89               = detail::Palette256Color { 254 };
constexpr TextColor GREY93               = detail::Palette256Color { 255 };
constexpr TextColor HONEY_DEW2           = detail::Palette256Color { 194 };
constexpr TextColor HOT_PINK             = detail::Palette256Color { 205 };
constexpr TextColor HOT_PINK2            = detail::Palette256Color { 169 };
constexpr TextColor HOT_PINK3            = detail::Palette256Color { 132 };
constexpr TextColor HOT_PINK3BIS         = detail::Palette256Color { 168 };
constexpr TextColor HOT_PINKBIS          = detail::Palette256Color { 206 };
constexpr TextColor INDIAN_RED           = detail::Palette256Color { 131 };
constexpr TextColor INDIAN_RED1          = detail::Palette256Color { 203 };
constexpr TextColor INDIAN_RED1BIS       = detail::Palette256Color { 204 };
constexpr TextColor INDIAN_REDBIS        = detail::Palette256Color { 167 };
constexpr TextColor KHAKI1               = detail::Palette256Color { 228 };
constexpr TextColor KHAKI3               = detail::Palette256Color { 185 };
constexpr TextColor LIGHT_CORAL          = detail::Palette256Color { 210 };
constexpr TextColor LIGHT_CYAN1BIS       = detail::Palette256Color { 195 };
constexpr TextColor LIGHT_CYAN3          = detail::Palette256Color { 152 };
constexpr TextColor LIGHT_GOLDEN_ROD1    = detail::Palette256Color { 227 };
constexpr TextColor LIGHT_GOLDEN_ROD2    = detail::Palette256Color { 186 };
constexpr TextColor LIGHT_GOLDEN_ROD2BIS = detail::Palette256Color { 221 };
constexpr TextColor LIGHT_GOLDEN_ROD2TER = detail::Palette256Color { 222 };
constexpr TextColor LIGHT_GOLDEN_ROD3    = detail::Palette256Color { 179 };
constexpr TextColor LIGHT_GREEN          = detail::Palette256Color { 119 };
constexpr TextColor LIGHT_GREENBIS       = detail::Palette256Color { 120 };
constexpr TextColor LIGHT_PINK1          = detail::Palette256Color { 217 };
constexpr TextColor LIGHT_PINK3          = detail::Palette256Color { 174 };
constexpr TextColor LIGHT_PINK4          = detail::Palette256Color { 95 };
constexpr TextColor LIGHT_SALMON1        = detail::Palette256Color { 216 };
constexpr TextColor LIGHT_SALMON3        = detail::Palette256Color { 137 };
constexpr TextColor LIGHT_SALMON3BIS     = detail::Palette256Color { 173 };
constexpr TextColor LIGHT_SEA_GREEN      = detail::Palette256Color { 37 };
constexpr TextColor LIGHT_SKY_BLUE1      = detail::Palette256Color { 153 };
constexpr TextColor LIGHT_SKY_BLUE3      = detail::Palette256Color { 109 };
constexpr TextColor LIGHT_SKY_BLUE3BIS   = detail::Palette256Color { 110 };
constexpr TextColor LIGHT_SLATE_BLUE     = detail::Palette256Color { 105 };
constexpr TextColor LIGHT_SLATE_GREY     = detail::Palette256Color { 103 };
constexpr TextColor LIGHT_STEEL_BLUE     = detail::Palette256Color { 147 };
constexpr TextColor LIGHT_STEEL_BLUE1    = detail::Palette256Color { 189 };
constexpr TextColor LIGHT_STEEL_BLUE3    = detail::Palette256Color { 146 };
constexpr TextColor LIGHT_YELLOW3        = detail::Palette256Color { 187 };
constexpr TextColor MAGENTA1             = detail::Palette256Color { 201 };
constexpr TextColor MAGENTA2             = detail::Palette256Color { 165 };
constexpr TextColor MAGENTA2BIS          = detail::Palette256Color { 200 };
constexpr TextColor MAGENTA3             = detail::Palette256Color { 127 };
constexpr TextColor MAGENTA3BIS          = detail::Palette256Color { 163 };
constexpr TextColor MAGENTA3TER          = detail::Palette256Color { 164 };
constexpr TextColor MEDIUM_ORCHID        = detail::Palette256Color { 134 };
constexpr TextColor MEDIUM_ORCHID1       = detail::Palette256Color { 171 };
constexpr TextColor MEDIUM_ORCHID1BIS    = detail::Palette256Color { 207 };
constexpr TextColor MEDIUM_ORCHID3       = detail::Palette256Color { 133 };
constexpr TextColor MEDIUM_PURPLE        = detail::Palette256Color { 104 };
constexpr TextColor MEDIUM_PURPLE1       = detail::Palette256Color { 141 };
constexpr TextColor MEDIUM_PURPLE2       = detail::Palette256Color { 135 };
constexpr TextColor MEDIUM_PURPLE2BIS    = detail::Palette256Color { 140 };
constexpr TextColor MEDIUM_PURPLE3       = detail::Palette256Color { 97 };
constexpr TextColor MEDIUM_PURPLE3BIS    = detail::Palette256Color { 98 };
constexpr TextColor MEDIUM_PURPLE4       = detail::Palette256Color { 60 };
constexpr TextColor MEDIUM_SPRING_GREEN  = detail::Palette256Color { 49 };
constexpr TextColor MEDIUM_TURQUOISE     = detail::Palette256Color { 80 };
constexpr TextColor MEDIUM_VIOLET_RED    = detail::Palette256Color { 126 };
constexpr TextColor MISTYROSE1           = detail::Palette256Color { 224 };
constexpr TextColor MISTYROSE3           = detail::Palette256Color { 181 };
constexpr TextColor NAVAJO_WHITE1        = detail::Palette256Color { 223 };
constexpr TextColor NAVAJO_WHITE3        = detail::Palette256Color { 144 };
constexpr TextColor NAVY_BLUE            = detail::Palette256Color { 17 };
constexpr TextColor ORANGE1              = detail::Palette256Color { 214 };
constexpr TextColor ORANGE3              = detail::Palette256Color { 172 };
constexpr TextColor ORANGE4              = detail::Palette256Color { 58 };
constexpr TextColor ORANGE4BIS           = detail::Palette256Color { 94 };
constexpr TextColor ORANGERED1           = detail::Palette256Color { 202 };
constexpr TextColor ORCHID               = detail::Palette256Color { 170 };
constexpr TextColor ORCHID1              = detail::Palette256Color { 213 };
constexpr TextColor ORCHID2              = detail::Palette256Color { 212 };
constexpr TextColor PALE_GREEN1          = detail::Palette256Color { 121 };
constexpr TextColor PALE_GREEN1BIS       = detail::Palette256Color { 156 };
constexpr TextColor PALE_GREEN3          = detail::Palette256Color { 114 };
constexpr TextColor PALE_GREEN3BIS       = detail::Palette256Color { 77 };
constexpr TextColor PALE_TURQUOISE1      = detail::Palette256Color { 159 };
constexpr TextColor PALE_TURQUOISE4      = detail::Palette256Color { 66 };
constexpr TextColor PALE_VIOLET_RED1     = detail::Palette256Color { 211 };
constexpr TextColor PINK1                = detail::Palette256Color { 218 };
constexpr TextColor PINK3                = detail::Palette256Color { 175 };
constexpr TextColor PLUM1                = detail::Palette256Color { 219 };
constexpr TextColor PLUM2                = detail::Palette256Color { 183 };
constexpr TextColor PLUM3                = detail::Palette256Color { 176 };
constexpr TextColor PLUM4                = detail::Palette256Color { 96 };
constexpr TextColor PURPLE               = detail::Palette256Color { 129 };
constexpr TextColor PURPLE3              = detail::Palette256Color { 56 };
constexpr TextColor PURPLE4              = detail::Palette256Color { 54 };
constexpr TextColor PURPLE4BIS           = detail::Palette256Color { 55 };
constexpr TextColor PURPLEBIS            = detail::Palette256Color { 93 };
constexpr TextColor RED1                 = detail::Palette256Color { 196 };
constexpr TextColor RED3                 = detail::Palette256Color { 124 };
constexpr TextColor RED3BIS              = detail::Palette256Color { 160 };
constexpr TextColor ROSY_BROWN           = detail::Palette256Color { 138 };
constexpr TextColor ROYAL_BLUE1          = detail::Palette256Color { 63 };
constexpr TextColor SALMON1              = detail::Palette256Color { 209 };
constexpr TextColor SANDY_BROWN          = detail::Palette256Color { 215 };
constexpr TextColor SEA_GREEN1           = detail::Palette256Color { 84 };
constexpr TextColor SEA_GREEN1BIS        = detail::Palette256Color { 85 };
constexpr TextColor SEA_GREEN2           = detail::Palette256Color { 83 };
constexpr TextColor SEA_GREEN3           = detail::Palette256Color { 78 };
constexpr TextColor SKY_BLUE1            = detail::Palette256Color { 117 };
constexpr TextColor SKY_BLUE2            = detail::Palette256Color { 111 };
constexpr TextColor SKY_BLUE3            = detail::Palette256Color { 74 };
constexpr TextColor SLATE_BLUE1          = detail::Palette256Color { 99 };
constexpr TextColor SLATE_BLUE3          = detail::Palette256Color { 61 };
constexpr TextColor SLATE_BLUE3BIS       = detail::Palette256Color { 62 };
constexpr TextColor SPRING_GREEN1        = detail::Palette256Color { 48 };
constexpr TextColor SPRING_GREEN2        = detail::Palette256Color { 42 };
constexpr TextColor SPRING_GREEN2BIS     = detail::Palette256Color { 47 };
constexpr TextColor SPRING_GREEN3        = detail::Palette256Color { 35 };
constexpr TextColor SPRING_GREEN3BIS     = detail::Palette256Color { 41 };
constexpr TextColor SPRING_GREEN4        = detail::Palette256Color { 29 };
constexpr TextColor STEEL_BLUE           = detail::Palette256Color { 67 };
constexpr TextColor STEEL_BLUE1          = detail::Palette256Color { 75 };
constexpr TextColor STEEL_BLUE1BIS       = detail::Palette256Color { 81 };
constexpr TextColor STEEL_BLUE3          = detail::Palette256Color { 68 };
constexpr TextColor TAN                  = detail::Palette256Color { 180 };
constexpr TextColor THISTLE1             = detail::Palette256Color { 225 };
constexpr TextColor THISTLE3             = detail::Palette256Color { 182 };
constexpr TextColor TURQUOISE2           = detail::Palette256Color { 45 };
constexpr TextColor TURQUOISE4           = detail::Palette256Color { 30 };
constexpr TextColor VIOLET               = detail::Palette256Color { 177 };
constexpr TextColor WHEAT1               = detail::Palette256Color { 229 };
constexpr TextColor WHEAT4               = detail::Palette256Color { 101 };
constexpr TextColor YELLOW1              = detail::Palette256Color { 226 };
constexpr TextColor YELLOW2              = detail::Palette256Color { 190 };
constexpr TextColor YELLOW3              = detail::Palette256Color { 148 };
constexpr TextColor YELLOW3BIS           = detail::Palette256Color { 184 };
constexpr TextColor YELLOW4              = detail::Palette256Color { 100 };
constexpr TextColor YELLOW4BIS           = detail::Palette256Color { 106 };
/*@formatter:on*/

}

template<>
struct std::hash<tui::detail::Palette16Color> {
  std::size_t operator()(tui::detail::Palette16Color const &c) const noexcept {
    return c.index;
  }
};

template<>
struct std::hash<tui::detail::Palette256Color> {
  std::size_t operator()(tui::detail::Palette256Color const &c) const noexcept {
    return c.index;
  }
};

template<>
struct std::hash<tui::detail::TrueColor> {
  std::size_t operator()(tui::detail::TrueColor const &c) const noexcept {
    return c.value;
  }
};
