#include <tui++/terminal/TerminalTheme.h>

#include <tui++/lookandfeel/SystemColorKeys.h>

#include <tui++/Insets.h>
#include <tui++/border/LineBorder.h>

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

  auto control = get<std::optional<Color>>(SystemColorKeys::CONTROL);
  auto controlDkShadow = get<std::optional<Color>>(SystemColorKeys::CONTROL_DK_SHADOW);
  auto controlHighlight = get<std::optional<Color>>(SystemColorKeys::CONTROL_HIGHLIGHT);
  auto controlLtHighlight = get<std::optional<Color>>(SystemColorKeys::CONTROL_LT_HIGHLIGHT);
  auto controlShadow = get<std::optional<Color>>(SystemColorKeys::CONTROL_SHADOW);
  auto controlText = get<std::optional<Color>>(SystemColorKeys::CONTROL_TEXT);
  auto menu = get<std::optional<Color>>(SystemColorKeys::MENU);
  auto menuText = get<std::optional<Color>>(SystemColorKeys::MENU_TEXT);
  auto textHighlight = get<std::optional<Color>>(SystemColorKeys::TEXT_HIGHLIGHT);
  auto textHighlightText = get<std::optional<Color>>(SystemColorKeys::TEXT_HIGHLIGHT_TEXT);
  auto textInactiveText = get<std::optional<Color>>(SystemColorKeys::TEXT_INACTIVE_TEXT);
  auto textText = get<std::optional<Color>>(SystemColorKeys::TEXT_TEXT);
  auto window = get<std::optional<Color>>(SystemColorKeys::WINDOW);

  auto zeroInsets = make_resource<Insets>(0, 0, 0, 0);
  auto twoInsets = make_resource<Insets>(2, 2, 2, 2);
  auto threeInsets = make_resource<Insets>(3, 3, 3, 3);

//  put("Button.border", []{
//    return std::make_shared<LineBorder>(Stroke::DOUBLE, RED_COLOR);
//  });
}

}
