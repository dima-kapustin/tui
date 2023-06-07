#pragma once

namespace tui {

enum class Cursor {
  BLOCK_BLINKING = 1,
  STEADY_BLOCK = 2,
  UNDERLINE_BLINKING = 3,
  STEADY_UNDERLINE = 4,
  BAR_BLINKING = 5,
  STEADY_BAR = 6,

  DEFAULT = BLOCK_BLINKING
};

}
