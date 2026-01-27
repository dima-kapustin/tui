#pragma once

#include <tui++/Themable.h>

namespace tui {

enum class CursorType {
  BLOCK_BLINKING = 1,
  STEADY_BLOCK = 2,
  UNDERLINE_BLINKING = 3,
  STEADY_UNDERLINE = 4,
  BAR_BLINKING = 5,
  STEADY_BAR = 6,

  DEFAULT = BLOCK_BLINKING
};

class Cursor: public Themable {
  CursorType type_ = CursorType::DEFAULT;

public:
  constexpr bool operator==(Cursor const &other) const {
    return this->type_ == other.type_;
  }

  constexpr auto type() const {
    return this->type_;
  }
};

}
