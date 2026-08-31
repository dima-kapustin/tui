#include <tui++/border/LineBorder.h>

#include <tui++/Graphics.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>

namespace tui {

Insets LineBorder::get_border_insets(const Component &c) const {
  // The text screen draws every stroke one cell thick; the graphic screen
  // draws them in pixels (LIGHT/DASHED 1, HEAVY/BOLD 2, DOUBLE 3). The
  // insets must match so the content does not overlap the border.
  if (screen.get_text_metrics()->get_line_height() == 1) {
    return {1, 1, 1, 1};
  }
  switch (this->stroke) {
  case Stroke::LIGHT:
  case Stroke::DASHED:
    return {1, 1, 1, 1};
  case Stroke::HEAVY:
  case Stroke::BOLD:
    return {2, 2, 2, 2};
  case Stroke::DOUBLE:
    return {3, 3, 3, 3};
  }
  return {1, 1, 1, 1};
}

void LineBorder::paint_border(const Component &c, Graphics &g, int x, int y, int width, int height) const {
  g.set_stroke(this->stroke);
  g.set_foreground_color(this->line_color);
  g.set_background_color(this->background_color);
  g.draw_rect(x, y, width, height);
}

}
