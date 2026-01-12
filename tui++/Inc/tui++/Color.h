#pragma once

#include <tui++/Theme.h>

#include <cstdint>

namespace tui {

class Color: public Themable {
  uint8_t blue_;
  uint8_t green_;
  uint8_t red_;

public:
  constexpr Color(uint8_t red, uint8_t green, uint8_t blue) :
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

public:
  constexpr bool operator==(Color const &other) const {
    return blue() == other.blue() and green() == other.green() and red() == other.red();
  }
};

constexpr Color BLACK_COLOR { 0, 0, 0 };
constexpr Color RED_COLOR { 128, 0, 0 };
constexpr Color GREEN_COLOR { 0, 128, 0 };
constexpr Color YELLOW_COLOR { 128, 128, 0 };
constexpr Color BLUE_COLOR { 0, 0, 128 };
constexpr Color MAGENTA_COLOR { 128, 0, 128 };
constexpr Color CYAN_COLOR { 0, 128, 128 };
constexpr Color GREY_DARK_COLOR { 128, 128, 128 };
constexpr Color BRIGHT_GREY_COLOR { 192, 192, 192 };
constexpr Color BRIGHT_RED_COLOR { 255, 0, 0 };
constexpr Color BRIGHT_GREEN_COLOR { 0, 255, 0 };
constexpr Color BRIGHT_YELLOW_COLOR { 255, 255, 0 };
constexpr Color BRIGHT_BLUE_COLOR { 0, 0, 255 };
constexpr Color BRIGHT_MAGENTA_COLOR { 255, 0, 255 };
constexpr Color BRIGHT_CYAN_COLOR { 0, 255, 255 };
constexpr Color WHITE_COLOR { 255, 255, 255 };

}
