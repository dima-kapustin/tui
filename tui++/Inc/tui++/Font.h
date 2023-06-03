#pragma once

#include <tui++/EnumMask.h>

namespace tui {

/**
 * The only useful information the Font class holds is whether the style is BOLD or not.
 */
class Font {
public:
  enum StyleFlags {
    PLAIN = 0,
    BOLD = 1,
    ITALIC = 2
  };

  using Style = EnumMask<StyleFlags>;

private:
  Style style = PLAIN;

public:
  Style get_style() const {
    return this->style;
  }
};

}
