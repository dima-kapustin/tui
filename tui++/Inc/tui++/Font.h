#pragma once

namespace tui {

/**
 * The only useful information the Font class holds is whether the style is BOLD or not.
 */
class Font {
public:
  enum Style {
    PLAIN = 0,
    BOLD = 1,
    ITALIC = 2
  };

  Style style = PLAIN;
};

}
