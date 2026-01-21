#include <tui++/SystemColors.h>

#include <utility>

#ifdef _WIN32
# include <windows.h>
#endif

namespace tui {

// @formatter:off
static Color system_colors[] = {
  0x005C5C_rgb, //
  0x000080_rgb, //
  0xFFFFFF_rgb, //
  0xC0C0C0_rgb, //
  0x808080_rgb, //
  0xC0C0C0_rgb, //
  0xC0C0C0_rgb, //
  0xFFFFFF_rgb, //
  0x000000_rgb, //
  0x000000_rgb, //
  0xC0C0C0_rgb, //
  0x000000_rgb, //
  0xC0C0C0_rgb, //
  0x000000_rgb, //
  0x000080_rgb, //
  0xFFFFFF_rgb, //
  0x808080_rgb, //
  0xC0C0C0_rgb, //
  0x000000_rgb, //
  0xFFFFFF_rgb, //
  0xE0E0E0_rgb, //
  0x808080_rgb, //
  0x000000_rgb, //
  0xE0E0E0_rgb, //
  0xE0E000_rgb, //
  0x000000_rgb, //
};

inline Color get_system_color(SystemColorIndex index) {
  return system_colors[std::to_underlying(index)];
}

Color const &DESKTOP_COLOR                  = get_system_color(SystemColorIndex::DESKTOP);
Color const &ACTIVE_CAPTION_COLOR           = get_system_color(SystemColorIndex::ACTIVE_CAPTION);
Color const &ACTIVE_CAPTION_TEXT_COLOR      = get_system_color(SystemColorIndex::ACTIVE_CAPTION_TEXT);
Color const &ACTIVE_CAPTION_BORDER_COLOR    = get_system_color(SystemColorIndex::ACTIVE_CAPTION_BORDER);
Color const &INACTIVE_CAPTION_COLOR         = get_system_color(SystemColorIndex::INACTIVE_CAPTION);
Color const &INACTIVE_CAPTION_TEXT_COLOR    = get_system_color(SystemColorIndex::INACTIVE_CAPTION_TEXT);
Color const &INACTIVE_CAPTION_BORDER_COLOR  = get_system_color(SystemColorIndex::INACTIVE_CAPTION_BORDER);
Color const &WINDOW_COLOR                   = get_system_color(SystemColorIndex::WINDOW);
Color const &WINDOW_BORDER_COLOR            = get_system_color(SystemColorIndex::WINDOW_BORDER);
Color const &WINDOW_TEXT_COLOR              = get_system_color(SystemColorIndex::WINDOW_TEXT);
Color const &MENU_COLOR                     = get_system_color(SystemColorIndex::MENU);
Color const &MENU_TEXT_COLOR                = get_system_color(SystemColorIndex::MENU_TEXT);
Color const &TEXT_COLOR                     = get_system_color(SystemColorIndex::TEXT);
Color const &TEXT_TEXT_COLOR                = get_system_color(SystemColorIndex::TEXT_TEXT);
Color const &TEXT_HIGHLIGHT_COLOR           = get_system_color(SystemColorIndex::TEXT_HIGHLIGHT);
Color const &TEXT_HIGHLIGHT_TEXT_COLOR      = get_system_color(SystemColorIndex::TEXT_HIGHLIGHT_TEXT);
Color const &TEXT_INACTIVE_TEXT_COLOR       = get_system_color(SystemColorIndex::TEXT_INACTIVE_TEXT);
Color const &CONTROL_COLOR                  = get_system_color(SystemColorIndex::CONTROL);
Color const &CONTROL_TEXT_COLOR             = get_system_color(SystemColorIndex::CONTROL_TEXT);
Color const &CONTROL_HIGHLIGHT_COLOR        = get_system_color(SystemColorIndex::CONTROL_HIGHLIGHT);
Color const &CONTROL_LT_HIGHLIGHT_COLOR     = get_system_color(SystemColorIndex::CONTROL_LT_HIGHLIGHT);
Color const &CONTROL_SHADOW_COLOR           = get_system_color(SystemColorIndex::CONTROL_SHADOW);
Color const &CONTROL_DK_SHADOW_COLOR        = get_system_color(SystemColorIndex::CONTROL_DK_SHADOW);
Color const &SCROLLBAR_COLOR                = get_system_color(SystemColorIndex::SCROLLBAR);
Color const &INFO_COLOR                     = get_system_color(SystemColorIndex::INFO);
Color const &INFO_TEXT_COLOR                = get_system_color(SystemColorIndex::INFO_TEXT);
// @formatter:on

void update_system_colors() {
#ifdef _WIN32
  static int index_map[] = { //
      COLOR_DESKTOP, /* DESKTOP */
      COLOR_ACTIVECAPTION, /* ACTIVE_CAPTION */
      COLOR_CAPTIONTEXT, /* ACTIVE_CAPTION_TEXT */
      COLOR_ACTIVEBORDER, /* ACTIVE_CAPTION_BORDER */
      COLOR_INACTIVECAPTION, /* INACTIVE_CAPTION */
      COLOR_INACTIVECAPTIONTEXT, /* INACTIVE_CAPTION_TEXT */
      COLOR_INACTIVEBORDER, /* INACTIVE_CAPTION_BORDER */
      COLOR_WINDOW, /* WINDOW */
      COLOR_WINDOWFRAME, /* WINDOW_BORDER */
      COLOR_WINDOWTEXT, /* WINDOW_TEXT */
      COLOR_MENU, /* MENU */
      COLOR_MENUTEXT, /* MENU_TEXT */
      COLOR_WINDOW, /* TEXT */
      COLOR_WINDOWTEXT, /* TEXT_TEXT */
      COLOR_HIGHLIGHT, /* TEXT_HIGHLIGHT */
      COLOR_HIGHLIGHTTEXT, /* TEXT_HIGHLIGHT_TEXT */
      COLOR_GRAYTEXT, /* TEXT_INACTIVE_TEXT */
      COLOR_3DFACE, /* CONTROL */
      COLOR_BTNTEXT, /* CONTROL_TEXT */
      COLOR_3DLIGHT, /* CONTROL_HIGHLIGHT */
      COLOR_3DHILIGHT, /* CONTROL_LT_HIGHLIGHT */
      COLOR_3DSHADOW, /* CONTROL_SHADOW */
      COLOR_3DDKSHADOW, /* CONTROL_DK_SHADOW */
      COLOR_SCROLLBAR, /* SCROLLBAR */
      COLOR_INFOBK, /* INFO */
      COLOR_INFOTEXT, /* INFO_TEXT */
  };

  for (auto i = 0U; i != std::size(index_map); ++i) {
    auto sys_color = ::GetSysColor(index_map[i]);
    system_colors[i] = { GetRValue(sys_color), GetGValue(sys_color), GetBValue(sys_color) };
  }
#endif
}

}
