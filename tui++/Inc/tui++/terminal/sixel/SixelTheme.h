#pragma once

#include <tui++/terminal/text/TextTheme.h>

#include <tui++/border/BevelBorder.h>
#include <tui++/border/CompoundBorder.h>
#include <tui++/border/LineBorder.h>

#include <tui++/lookandfeel/SystemColorKeys.h>
#include <tui++/lookandfeel/basic/ButtonBorder.h>

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

    // The chrome is drawn in pixels here, so the button keeps its full box
    // bezel and the dialog its raised internal-frame box: the lines are one
    // pixel thick and cost the label no row of its own the way the text
    // theme's cell-sized edges would (see ButtonBorder::Shape).
    auto button_border = BorderFactory { [this] {
      return make_shared_resource<laf::ButtonBorder>( //
          get_color(SystemColorKeys::CONTROL_SHADOW), //
          get_color(SystemColorKeys::CONTROL_DK_SHADOW), //
          get_color(SystemColorKeys::CONTROL_HIGHLIGHT), //
          get_color(SystemColorKeys::CONTROL_LT_HIGHLIGHT), //
          laf::ButtonBorder::Shape::BOX);
    } };
    put("Button.Border", button_border);
    put("ToggleButton.Border", button_border);

    auto dialog_border = BorderFactory { [this] {
      static auto border = make_shared_resource<CompoundBorder>( //
          std::make_shared<BevelBorder>( //
              BevelBorder::RAISED, //
              get_color("InternalFrame.BorderLight"), //
              get_color("InternalFrame.BorderHighlight"), //
              get_color("InternalFrame.BorderDarkShadow"), //
              get_color("InternalFrame.BorderShadow")), //
          std::make_shared<LineBorder>( //
              Stroke::LIGHT, //
              get_color("InternalFrame.BorderColor")));
      return border;
    } };
    put("Dialog.Border", dialog_border);
  }
};

}
