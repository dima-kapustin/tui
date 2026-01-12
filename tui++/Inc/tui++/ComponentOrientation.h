#pragma once

#include <locale>

namespace tui {

class ComponentOrientation final {
  class Impl {
    constexpr static int UNK_BIT = 1;
    constexpr static int HORIZ_BIT = 2;
    constexpr static int LTR_BIT = 4;

    enum Orientation {
      LEFT_TO_RIGHT = HORIZ_BIT | LTR_BIT,
      RIGHT_TO_LEFT = HORIZ_BIT,
      UNKNOWN = HORIZ_BIT | LTR_BIT | UNK_BIT
    } orientation;

  private:
    constexpr Impl(Orientation orientation) :
        orientation(orientation) {
    }

    friend class ComponentOrientation;

  public:
    bool is_horizontal() const {
      return (this->orientation & HORIZ_BIT) != 0;
    }

    bool is_left_to_right() const {
      return (this->orientation & LTR_BIT) != 0;
    }

    constexpr bool operator==(Impl const&) const = default;
  };

private:
  Impl orientation;

public:
  static inline const Impl LEFT_TO_RIGHT { Impl::LEFT_TO_RIGHT };
  static inline const Impl RIGHT_TO_LEFT { Impl::RIGHT_TO_LEFT };
  static inline const Impl UNKNOWN { Impl::UNKNOWN };

public:
  ComponentOrientation(const Impl &impl) :
      orientation(impl) {
  }

  bool is_horizontal() const {
    return this->orientation.is_horizontal();
  }

  bool is_left_to_right() const {
    return this->orientation.is_left_to_right();
  }

  constexpr bool operator==(ComponentOrientation const&) const = default;
};

}
