#include <tui++/terminal/TerminalTheme.h>

#include <tui++/lookandfeel/SystemColorKeys.h>
#include <tui++/lookandfeel/border/MarginBorder.h>

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

  auto margin_border = BorderFactory { [] {
    return std::make_shared<laf::MarginBorder>();
  } };

//  put( { { "MenuItem.border", margin_border } });
//
//  auto border = get_border("MenuItem.border");

}

}
