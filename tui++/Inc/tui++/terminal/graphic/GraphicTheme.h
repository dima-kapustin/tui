#pragma once

#include <tui++/terminal/text/TextTheme.h>

#include <tui++/Insets.h>

namespace tui {

// Theme installed alongside the "graphic" (sixel) terminal. It shares the
// text theme's color and border defaults; the pixel-level differences in
// component sizing and layout live in GraphicLookAndFeel and its UI delegates.
class GraphicTheme: public TextTheme {
protected:
  virtual void init() override {
    TextTheme::init();
    // Menu label margins in pixels, matching Swing's BasicLookAndFeel; the
    // text screen keeps its one-cell margins from TextTheme.
    put("MenuItem.margin", make_resource<Insets>(2, 2, 2, 2));
    put("Menu.margin", make_resource<Insets>(2, 2, 2, 2));
  }
};

}
