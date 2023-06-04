#pragma once

#include <cmath>
#include <cstdint>
#include <variant>
#include <algorithm>

namespace tui {

struct ColorIndex {
  unsigned value;
};

constexpr ColorIndex BLACK_COLOR { 0 };
constexpr ColorIndex RED_COLOR { 1 };
constexpr ColorIndex GREEN_COLOR { 2 };
constexpr ColorIndex YELLOW_COLOR { 3 };
constexpr ColorIndex BLUE_COLOR { 4 };
constexpr ColorIndex MAGENTA_COLOR { 5 };
constexpr ColorIndex CYAN_COLOR { 6 };
constexpr ColorIndex WHITE_COLOR { 7 };
constexpr ColorIndex BRIGHT_BLACK_COLOR { 8 };
constexpr ColorIndex BRIGHT_RED_COLOR { 9 };
constexpr ColorIndex BRIGHT_GREEN_COLOR { 10 };
constexpr ColorIndex BRIGHT_YELLOW_COLOR { 11 };
constexpr ColorIndex BRIGHT_BLUE_COLOR { 12 };
constexpr ColorIndex BRIGHT_MAGENTA_COLOR { 13 };
constexpr ColorIndex BRIGHT_CYAN_COLOR { 14 };
constexpr ColorIndex BRIGHT_WHITE_COLOR { 15 };

struct DefaultColor {
};

constexpr DefaultColor DEFAULT_COLOR;

struct RGB {
  uint32_t red :8;
  uint32_t green :8;
  uint32_t blue :8;

  /// Direct contstruction with red, green and blue values.
  constexpr RGB(unsigned r, unsigned g, unsigned b) :
      red { r }, green { g }, blue { b } {
  }

  /// Takes a (hex) value and pulls out 8 byte segments for RGB.
  /** RGB{0x7f9860} 0x7f is red, 0x98 is green, 0x60 is blue. */
  constexpr RGB(unsigned hex) :
      red { (hex >> 16) & 0xFF }, green { (hex >> 8) & 0xFF }, blue { hex & 0xFF } {
  }
};

struct HSL {
  uint32_t hue :16;        // [0, 359] degrees
  uint32_t saturation :8;  // [0, 100] %
  uint32_t lightness :8;   // [0, 100] %
};

/// Convert HSL to RGB
constexpr RGB hsl_to_rgb(HSL v) {
  const auto lightness = v.lightness / 100.;
  const auto saturation = v.saturation / 100.;

  auto const c = (1 - std::abs((2 * lightness) - 1.)) * saturation;
  const auto h_prime = v.hue / 60.;
  const auto x = c * (1. - std::abs(std::fmod(h_prime, 2.) - 1.));
  const auto m = lightness - (c / 2.);

  const auto c_ = std::uint8_t((c + m) * 255);
  const auto x_ = std::uint8_t((x + m) * 255);
  const auto m_ = std::uint8_t(m * 255);

  if (v.hue < 60.)
    return {c_, x_, m_};
  if (v.hue < 120.)
    return {x_, c_, m_};
  if (v.hue < 180.)
    return {m_, c_, x_};
  if (v.hue < 240.)
    return {m_, x_, c_};
  if (v.hue < 300.)
    return {x_, m_, c_};
  if (v.hue < 360.)
    return {c_, m_, x_};
  else
    return {0, 0, 0};
}

/// Convert RGB to HSL
constexpr HSL rgb_to_hsl(RGB x) {
  const auto r_prime = x.red / 255.;
  const auto g_prime = x.green / 255.;
  const auto b_prime = x.blue / 255.;

  const auto c_max = std::max( { r_prime, g_prime, b_prime });
  const auto c_min = std::min( { r_prime, g_prime, b_prime });
  const auto delta = c_max - c_min;

  const auto lightness = (c_max + c_min) / 2.;
  const auto saturation = [&] {
    if (delta == 0.)
      return 0.;
    double const den = 1. - std::abs(2. * lightness - 1.);
    return delta / den;
  }();

  const auto hue = [&] {
    if (delta == 0.)
      return 0.;
    if (c_max == r_prime)
      return 60. * std::fmod((g_prime - b_prime) / delta, 6.);
    if (c_max == g_prime)
      return 60. * (((b_prime - r_prime) / delta) + 2.);
    if (c_max == b_prime)
      return 60. * (((r_prime - g_prime) / delta) + 4.);
    return 0.;
  }();

  return {std::uint16_t(hue), std::uint8_t(saturation * 100), std::uint8_t(lightness * 100)};
}

struct TrueColor {
  uint32_t red :8;
  uint32_t green :8;
  uint32_t blue :8;

  constexpr TrueColor(RGB x) :
      red { x.red }, green { x.green }, blue { x.blue } {
  }

  constexpr TrueColor(HSL x) :
      TrueColor { hsl_to_rgb(x) } {
  }
};

using Color = std::variant<ColorIndex, DefaultColor, TrueColor>;

}
