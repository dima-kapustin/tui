#pragma once

#include <tui++/terminal/text/TextTheme.h>

#include <tui++/Insets.h>
#include <tui++/Shadow.h>

namespace tui {

// Theme installed alongside the "sixel" (sixel) terminal. It shares the
// text theme's color and border defaults; the pixel-level differences in
// component sizing and layout live in SixelLookAndFeel and its UI delegates.
class SixelTheme: public TextTheme {
protected:
  virtual void init() override {
    TextTheme::init();
    // Menu label margins in pixels, matching Swing's BasicLookAndFeel; the
    // text screen keeps its one-cell margins from TextTheme.
    put("MenuItem.margin", make_resource<Insets>(2, 2, 2, 2));
    put("Menu.margin", make_resource<Insets>(2, 2, 2, 2));
    put("ComboBox.margin", make_resource<Insets>(2, 2, 2, 2));
    put("PopupMenuSeparator.margin", make_resource<Insets>(2, 2, 2, 2));

    // The drop shadows are measured in pixels here. The text theme's two
    // cells sideways and one cell down would be a 32x32 pixel block on the
    // graphic font's 16x32 cell -- half a cell in each direction reads as the
    // same shadow without weighing the popup down.
    auto popup_shadow = make_resource<Shadow>(BLACK_COLOR, 0.5, Point { 8, 6 });
    put("PopupMenu.Shadow", popup_shadow);
    put("ComboBox.Shadow", popup_shadow);
    put("Dialog.Shadow", popup_shadow);
  }
};

}
