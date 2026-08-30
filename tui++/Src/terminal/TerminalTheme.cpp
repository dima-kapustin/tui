#include <tui++/terminal/TerminalTheme.h>

#include <tui++/border/LineBorder.h>
#include <tui++/border/BevelBorder.h>
#include <tui++/border/EmptyBorder.h>
#include <tui++/border/EtchedBorder.h>
#include <tui++/border/CompoundBorder.h>

#include <tui++/lookandfeel/SystemColorKeys.h>
#include <tui++/lookandfeel/basic/MarginBorder.h>
#include <tui++/lookandfeel/basic/ButtonBorder.h>

#include <tui++/Insets.h>

namespace tui {

void TerminalTheme::init() {
  put( //
      { { "Menu.SubmenuPopupOffsetX", 0 }, //
        { "Menu.SubmenuPopupOffsetY", 0 }, //
        { "Menu.MenuPopupOffsetX", 0 }, //
        { "Menu.MenuPopupOffsetY", 0 }, //
      });

  init_system_color_defaults();
  init_component_defaults();
}

void TerminalTheme::init_system_color_defaults() {
  update_system_colors();

  using namespace SystemColorKeys;
  for (auto &&key : SYSTEM_COLOR_KEYS) {
    put(key, make_resource(get_system_color(key)));
  }
}

void TerminalTheme::init_component_defaults() {
  auto red = make_resource(RED_COLOR);
  auto black = make_resource(BLACK_COLOR);
  auto white = make_resource(WHITE_COLOR);
  auto yellow = make_resource(YELLOW_COLOR);
  auto gray = make_resource(GRAY_COLOR);
  auto lightGray = make_resource(LIGHT_GRAY_COLOR);
  auto darkGray = make_resource(DARK_GRAY_COLOR);
  auto scrollBarTrack = make_resource<Color>(224, 224, 224);

  auto control = get_color(SystemColorKeys::CONTROL);
  auto controlDkShadow = get_color(SystemColorKeys::CONTROL_DK_SHADOW);
  auto controlHighlight = get_color(SystemColorKeys::CONTROL_HIGHLIGHT);
  auto controlLtHighlight = get_color(SystemColorKeys::CONTROL_LT_HIGHLIGHT);
  auto controlShadow = get_color(SystemColorKeys::CONTROL_SHADOW);
  auto controlText = get_color(SystemColorKeys::CONTROL_TEXT);
  auto menu = get_color(SystemColorKeys::MENU);
  auto menuText = get_color(SystemColorKeys::MENU_TEXT);
  auto textHighlight = get_color(SystemColorKeys::TEXT_HIGHLIGHT);
  auto textHighlightText = get_color(SystemColorKeys::TEXT_HIGHLIGHT_TEXT);
  auto textInactiveText = get_color(SystemColorKeys::TEXT_INACTIVE_TEXT);
  auto textText = get_color(SystemColorKeys::TEXT_TEXT);
  auto window = get_color(SystemColorKeys::WINDOW);

  auto zero_insets = make_resource<Insets>(0, 0, 0, 0);
  auto two_insets = make_resource<Insets>(2, 2, 2, 2);
  auto three_insets = make_resource<Insets>(3, 3, 3, 3);

  auto margin_border = BorderFactory { [this] {
    return make_shared_resource<laf::MarginBorder>();
  } };

  auto etched_border = BorderFactory { [this] {
    return make_shared_resource<EtchedBorder>();
  } };

  auto lowered_bevel_border = BorderFactory { [this] {
    return make_shared_resource<BevelBorder>(BevelBorder::LOWERED);
  } };

  auto internal_frame_border = BorderFactory { [this] {
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

  auto black_line_border = BorderFactory { [this] {
    return make_shared_resource<LineBorder>(Stroke::LIGHT, BLACK_COLOR);
  } };

  auto popup_menu_border = internal_frame_border;

  auto focusCellHighlightBorder = BorderFactory { [this] {
    return make_shared_resource<LineBorder>(Stroke::LIGHT, YELLOW_COLOR);
  } };

  auto noFocusBorder = make_shared_resource<EmptyBorder>(1, 1, 1, 1);

  auto tableHeaderBorder = BorderFactory { [this] {
    return make_shared_resource<BevelBorder>( //
        BevelBorder::RAISED, //
        get_color(SystemColorKeys::CONTROL_LT_HIGHLIGHT), //
        get_color(SystemColorKeys::CONTROL), //
        get_color(SystemColorKeys::CONTROL_DK_SHADOW), //
        get_color(SystemColorKeys::CONTROL_SHADOW));
  } };

  auto button_border = BorderFactory { [this] {
    return make_shared_resource<laf::ButtonBorder>( //
        get_color(SystemColorKeys::CONTROL_SHADOW), //
        get_color(SystemColorKeys::CONTROL_DK_SHADOW), //
        get_color(SystemColorKeys::CONTROL_HIGHLIGHT), //
        get_color(SystemColorKeys::CONTROL_LT_HIGHLIGHT));
  } };

  put("Button.Border", button_border);

//  put( { { "MenuItem.border", margin_border } });
//
//  auto border = get_border("MenuItem.border");

}

}
