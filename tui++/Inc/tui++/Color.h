#pragma once

#include <tui++/Theme.h>

#include <cstdint>
#include <charconv>

namespace tui {

class Color: public Themable {
  uint8_t blue_;
  uint8_t green_;
  uint8_t red_;

  constexpr static auto FACTOR = 0.7;

public:
  constexpr Color(uint8_t red, uint8_t green, uint8_t blue) :
      blue_(blue), green_(green), red_(red) {
  }

  constexpr Color(int red, int green, int blue) :
      blue_(blue), green_(green), red_(red) {
  }

  constexpr Color(Color const&) = default;
  constexpr Color(Color&&) = default;

  constexpr Color& operator=(Color const&) = default;
  constexpr Color& operator=(Color&&) = default;

public:
  constexpr uint8_t red() const {
    return this->red_;
  }

  constexpr uint8_t green() const {
    return this->green_;
  }

  constexpr uint8_t blue() const {
    return this->blue_;
  }

  constexpr uint8_t alpha() const {
    return 255;
  }

  constexpr Color brighter() const {
    auto r = red();
    auto g = green();
    auto b = blue();
    auto a = alpha();

    /* From 2D group:
     * 1. black.brighter() should return grey
     * 2. applying brighter to blue will always return blue, brighter
     * 3. non pure color (non zero rgb) will eventually return white
     */
    auto i = (int) (1.0 / (1.0 - FACTOR));
    if (r == 0 and g == 0 and b == 0) {
      return Color(i, i, i, a);
    }
    if (r != 0 and r < i)
      r = i;
    if (g != 0 and g < i)
      g = i;
    if (b != 0 and b < i)
      b = i;

    return { //
      std::min(int(r / FACTOR), 255),//
      std::min(int(g / FACTOR), 255),//
      std::min(int(b / FACTOR), 255),//
      a};
  }

  constexpr Color darker() const {
    return { //
      std::max(int(red() * FACTOR), 0),//
      std::max(int(green() * FACTOR), 0),//
      std::max(int(blue() * FACTOR), 0),//
      alpha()};
  }

public:
  constexpr bool operator==(Color const &other) const {
    return blue() == other.blue() and green() == other.green() and red() == other.red();
  }

private:
  constexpr Color(int red, int green, int blue, int alpha) :
      blue_(blue), green_(green), red_(red) {
  }
};

constexpr Color BLACK_COLOR { 0, 0, 0 };
constexpr Color RED_COLOR { 128, 0, 0 };
constexpr Color GREEN_COLOR { 0, 128, 0 };
constexpr Color YELLOW_COLOR { 128, 128, 0 };
constexpr Color BLUE_COLOR { 0, 0, 128 };
constexpr Color MAGENTA_COLOR { 128, 0, 128 };
constexpr Color CYAN_COLOR { 0, 128, 128 };
constexpr Color GRAY_COLOR { 128, 128, 128 };
constexpr Color DARK_GRAY_COLOR { 64, 64, 64 };
constexpr Color LIGHT_GRAY_COLOR { 192, 192, 192 };
constexpr Color LIGHT_RED_COLOR { 255, 0, 0 };
constexpr Color LIGHT_GREEN_COLOR { 0, 255, 0 };
constexpr Color LIGHT_YELLOW_COLOR { 255, 255, 0 };
constexpr Color LIGHT_BLUE_COLOR { 0, 0, 255 };
constexpr Color LIGHT_MAGENTA_COLOR { 255, 0, 255 };
constexpr Color LIGHT_CYAN_COLOR { 0, 255, 255 };
constexpr Color WHITE_COLOR { 255, 255, 255 };

namespace detail {

template<unsigned Base, unsigned Value>
struct DigitValue {
  static_assert(Value < Base);
  constexpr static unsigned VALUE = Value;
};

template<unsigned Base, char Digit>
struct NumberDigit;

template<unsigned Base>
struct NumberDigit<Base, '0'> : DigitValue<Base, 0> {
};

template<unsigned Base>
struct NumberDigit<Base, '1'> : DigitValue<Base, 1> {
};

template<unsigned Base>
struct NumberDigit<Base, '2'> : DigitValue<Base, 2> {
};

template<unsigned Base>
struct NumberDigit<Base, '3'> : DigitValue<Base, 3> {
};

template<unsigned Base>
struct NumberDigit<Base, '4'> : DigitValue<Base, 4> {
};

template<unsigned Base>
struct NumberDigit<Base, '5'> : DigitValue<Base, 5> {
};

template<unsigned Base>
struct NumberDigit<Base, '6'> : DigitValue<Base, 6> {
};

template<unsigned Base>
struct NumberDigit<Base, '7'> : DigitValue<Base, 7> {
};

template<unsigned Base>
struct NumberDigit<Base, '8'> : DigitValue<Base, 8> {
};

template<unsigned Base>
struct NumberDigit<Base, '9'> : DigitValue<Base, 9> {
};

template<unsigned Base>
struct NumberDigit<Base, 'a'> : DigitValue<Base, 0xa> {
};

template<unsigned Base>
struct NumberDigit<Base, 'A'> : DigitValue<Base, 0xa> {
};

template<unsigned Base>
struct NumberDigit<Base, 'b'> : DigitValue<Base, 0xb> {
};

template<unsigned Base>
struct NumberDigit<Base, 'B'> : DigitValue<Base, 0xb> {
};

template<unsigned Base>
struct NumberDigit<Base, 'c'> : DigitValue<Base, 0xc> {
};

template<unsigned Base>
struct NumberDigit<Base, 'C'> : DigitValue<Base, 0xc> {
};

template<unsigned Base>
struct NumberDigit<Base, 'd'> : DigitValue<Base, 0xd> {
};

template<unsigned Base>
struct NumberDigit<Base, 'D'> : DigitValue<Base, 0xd> {
};

template<unsigned Base>
struct NumberDigit<Base, 'e'> : DigitValue<Base, 0xe> {
};

template<unsigned Base>
struct NumberDigit<Base, 'E'> : DigitValue<Base, 0xe> {
};

template<unsigned Base>
struct NumberDigit<Base, 'f'> : DigitValue<Base, 0xf> {
};

template<unsigned Base>
struct NumberDigit<Base, 'F'> : DigitValue<Base, 0xf> {
};

template<unsigned Base, char ... Digits>
struct Number;

template<unsigned Base, char Digit>
struct Number<Base, Digit> {
  constexpr static unsigned POWER = 1;
  constexpr static unsigned VALUE = NumberDigit<Base, Digit>::VALUE;
};

template<unsigned Base>
struct Number<Base, '\''> {
  constexpr static unsigned POWER = 0;
  constexpr static unsigned VALUE = 0;
};

template<unsigned Base, char Digit, char ... Digits>
struct Number<Base, Digit, Digits...> {
  constexpr static unsigned POWER = Number<Base, Digit>::POWER ? Base * Number<Base, Digits...>::POWER : Number<Base, Digits...>::POWER;
  constexpr static unsigned VALUE = POWER * Number<Base, Digit>::VALUE + Number<Base, Digits...>::VALUE;
};

template<char ... Digits>
struct ParseInteger;

template<char ... Digits>
struct ParseInteger<'0', 'x', Digits...> : Number<16, Digits...> {
};

template<char ... Digits>
struct ParseInteger<'0', 'X', Digits...> : Number<16, Digits...> {
};

template<size_t N>
struct RgbHexString { // e.g. #C0C0C0
  char buff[N] { };
  constexpr RgbHexString(char const (&s)[N]) {
    std::copy(s, s + N, this->buff);
  }

  constexpr auto begin() const {
    return this->buff;
  }

  constexpr auto end() const {
    return this->buff + N;
  }
};

}

template<char ... Digits>
constexpr Color operator ""_rgb() {
//  static_assert(sizeof...(Digits) == 8);
  auto value = detail::ParseInteger<Digits...>::VALUE;
  return {uint8_t(value >> 16), uint8_t(value >> 8), uint8_t(value >> 0)};
}

template<detail::RgbHexString s>
constexpr Color operator ""_rgb() {
  auto value = 0U;
  std::from_chars(s.begin() + 1, s.end(), value, 16);
  return {uint8_t(value >> 16), uint8_t(value >> 8), uint8_t(value >> 0)};
}

}
