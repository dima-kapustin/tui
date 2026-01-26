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

void EtchedBorder::paint_border_highlight(Graphics &g, std::optional<Color> const &c, int w, int h, int stkWidth) const {
  g.set_color(c);
  g.draw_rect(stkWidth / 2, stkWidth / 2, w - (2 * stkWidth), h - (2 * stkWidth));
}

void EtchedBorder::paint_border_shadow(Graphics &g, std::optional<Color> const &c, int w, int h, int stkWidth) const {
  g.set_color(c);
  g.draw_vline(((3 * stkWidth) / 2), ((3 * stkWidth) / 2), h - ((3 * stkWidth) / 2)); // left line
  g.draw_hline(((3 * stkWidth) / 2), ((3 * stkWidth) / 2), w - ((3 * stkWidth) / 2)); // top line

  g.draw_hline((stkWidth / 2), (stkWidth - stkWidth / 2), w - (stkWidth - stkWidth / 2)); // bottom line
  g.draw_vline(w - (stkWidth - stkWidth / 2), stkWidth / 2, h - (stkWidth - stkWidth / 2)); // right line
}

}
