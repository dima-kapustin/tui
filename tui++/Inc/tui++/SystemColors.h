#pragma once

#include <tui++/Color.h>

namespace tui {

enum class SystemColorIndex : unsigned {
  DESKTOP = 0,
  ACTIVE_CAPTION,
  ACTIVE_CAPTION_TEXT,
  ACTIVE_CAPTION_BORDER,
  INACTIVE_CAPTION,
  INACTIVE_CAPTION_TEXT,
  INACTIVE_CAPTION_BORDER,
  WINDOW,
  WINDOW_BORDER,
  WINDOW_TEXT,
  MENU,
  MENU_TEXT,
  TEXT,
  TEXT_TEXT,
  TEXT_HIGHLIGHT,
  TEXT_HIGHLIGHT_TEXT,
  TEXT_INACTIVE_TEXT,
  CONTROL,
  CONTROL_TEXT,
  CONTROL_HIGHLIGHT,
  CONTROL_LT_HIGHLIGHT,
  CONTROL_SHADOW,
  CONTROL_DK_SHADOW,
  SCROLLBAR,
  INFO,
  INFO_TEXT
};

extern Color const &DESKTOP_COLOR;
extern Color const &ACTIVE_CAPTION_COLOR;
extern Color const &ACTIVE_CAPTION_TEXT_COLOR;
extern Color const &ACTIVE_CAPTION_BORDER_COLOR;
extern Color const &INACTIVE_CAPTION_COLOR;
extern Color const &INACTIVE_CAPTION_TEXT_COLOR;
extern Color const &INACTIVE_CAPTION_BORDER_COLOR;
extern Color const &WINDOW_COLOR;
extern Color const &WINDOW_BORDER_COLOR;
extern Color const &WINDOW_TEXT_COLOR;
extern Color const &MENU_COLOR;
extern Color const &MENU_TEXT_COLOR;
extern Color const &TEXT_COLOR;
extern Color const &TEXT_TEXT_COLOR;
extern Color const &TEXT_HIGHLIGHT_COLOR;
extern Color const &TEXT_HIGHLIGHT_TEXT_COLOR;
extern Color const &TEXT_INACTIVE_TEXT_COLOR;
extern Color const &CONTROL_COLOR;
extern Color const &CONTROL_TEXT_COLOR;
extern Color const &CONTROL_HIGHLIGHT_COLOR;
extern Color const &CONTROL_LT_HIGHLIGHT_COLOR;
extern Color const &CONTROL_SHADOW_COLOR;
extern Color const &CONTROL_DK_SHADOW_COLOR;
extern Color const &SCROLLBAR_COLOR;
extern Color const &INFO_COLOR;
extern Color const &INFO_TEXT_COLOR;

Color get_system_color(SystemColorIndex index);
void update_system_colors();

}
