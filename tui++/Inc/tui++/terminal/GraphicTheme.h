#pragma once

#include <tui++/terminal/TextTheme.h>

namespace tui {

// Theme installed alongside the "graphic" (sixel) terminal. It shares the
// text theme's color and border defaults; the pixel-level differences in
// component sizing and layout live in GraphicLookAndFeel and its UI delegates.
class GraphicTheme: public TextTheme {
};

}
