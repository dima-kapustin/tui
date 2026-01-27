#include <tui++/border/EtchedBorder.h>

#include <tui++/Graphics.h>
#include <tui++/Component.h>

namespace tui {

std::optional<Color> EtchedBorder::get_highlight_color(Component const &c) const {
  return this->highlight_color ? this->highlight_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.brighter();
  });
}

std::optional<Color> EtchedBorder::get_shadow_color(Component const &c) const {
  return this->shadow_color ? this->shadow_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.darker();
  });
}

void EtchedBorder::paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  auto stkWidth = 1; //(int) Math.floor(scaleFactor);
  g.set_stroke(Stroke::LIGHT);
  paint_border_shadow(g, (this->etch_type == EtchType::LOWERED) ? get_highlight_color(c) : get_shadow_color(c), width, height, stkWidth);
  paint_border_highlight(g, (this->etch_type == EtchType::LOWERED) ? get_shadow_color(c) : get_highlight_color(c), width, height, stkWidth);
}

void EtchedBorder::paint_border_highlight(Graphics &g, std::optional<Color> const &c, int w, int h, int stroke_width) const {
  g.set_color(c);
  g.draw_rect(stroke_width / 2, stroke_width / 2, w - (2 * stroke_width), h - (2 * stroke_width));
}

void EtchedBorder::paint_border_shadow(Graphics &g, std::optional<Color> const &c, int w, int h, int stroke_width) const {
  g.set_color(c);
  g.draw_vline(((3 * stroke_width) / 2), ((3 * stroke_width) / 2), h - ((3 * stroke_width) / 2)); // left line
  g.draw_hline(((3 * stroke_width) / 2), ((3 * stroke_width) / 2), w - ((3 * stroke_width) / 2)); // top line

  g.draw_hline((stroke_width / 2), (stroke_width - stroke_width / 2), w - (stroke_width - stroke_width / 2)); // bottom line
  g.draw_vline(w - (stroke_width - stroke_width / 2), stroke_width / 2, h - (stroke_width - stroke_width / 2)); // right line
}

}
