#include <tui++/terminal/TerminalTheme.h>

#include <tui++/lookandfeel/SystemColorKeys.h>

namespace tui {

void TerminalTheme::init() {
  put( //
      { { "Menu.SubmenuPopupOffsetX", 0 }, //
        { "Menu.SubmenuPopupOffsetY", 0 }, //
        { "Menu.MenuPopupOffsetX", 0 }, //
        { "Menu.MenuPopupOffsetY", 0 }, //
      });

  install_system_colors();
}

void TerminalTheme::install_system_colors() {
  update_system_colors();

  using namespace SystemColorKeys;
  for (auto &&key : SYSTEM_COLOR_KEYS) {
    put(key, make_resource(get_system_color(key)));
  }
}

}
