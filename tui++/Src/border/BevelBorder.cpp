#include <tui++/border/BevelBorder.h>

#include <tui++/Graphics.h>
#include <tui++/Component.h>

namespace tui {

Insets BevelBorder::get_border_insets(Component const &c) const {
  return {2, 2, 2, 2};
}

void BevelBorder::paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  if (this->bevel_type == BevelType::RAISED) {
    paint_raised_bevel(c, g, x, y, width, height);

  } else if (this->bevel_type == BevelType::LOWERED) {
    paint_lowered_bevel(c, g, x, y, width, height);
  }
}

void BevelBorder::paint_raised_bevel(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  auto h = height;
  auto w = width;

  g.translate(x, y);

  g.set_color(get_highlight_outer_color(c));
  g.draw_vline(0, 0, h - 2);
  g.draw_hline(1, 0, w - 2);

  g.set_color(get_highlight_inner_color(c));
  g.draw_vline(1, 1, h - 3);
  g.draw_hline(2, 1, w - 3);

  g.set_color(get_shadow_outer_color(c));
  g.draw_hline(0, h - 1, w - 1);
  g.draw_vline(w - 1, 0, h - 2);

  g.set_color(get_shadow_inner_color(c));
  g.draw_hline(1, h - 2, w - 2);
  g.draw_vline(w - 2, 1, h - 3);

  g.translate(-x, -y);
}

void BevelBorder::paint_lowered_bevel(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  auto h = height;
  auto w = width;

  g.translate(x, y);

  g.set_color(get_shadow_inner_color(c));
  g.draw_vline(0, 0, h - 1);
  g.draw_hline(1, 0, w - 1);

  g.set_color(get_shadow_outer_color(c));
  g.draw_vline(1, 1, h - 2);
  g.draw_hline(2, 1, w - 2);

  g.set_color(get_highlight_outer_color(c));
  g.draw_hline(1, h - 1, w - 1);
  g.draw_vline(w - 1, 1, h - 2);

  g.set_color(get_highlight_inner_color(c));
  g.draw_hline(2, h - 2, w - 2);
  g.draw_vline(w - 2, 2, h - 3);

  g.translate(-x, -y);
}

std::optional<Color> BevelBorder::get_highlight_outer_color(Component const &c) const {
  return this->highlight_outer_color ? this->highlight_outer_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.brighter().brighter();
  });
}

std::optional<Color> BevelBorder::get_highlight_inner_color(Component const &c) const {
  return this->highlight_inner_color ? this->highlight_inner_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.brighter();
  });
}

std::optional<Color> BevelBorder::get_shadow_inner_color(Component const &c) const {
  return this->shadow_inner_color ? this->shadow_inner_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.darker();
  });
}

std::optional<Color> BevelBorder::get_shadow_outer_color(Component const &c) const {
  return this->shadow_outer_color ? this->shadow_outer_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.darker().darker();
  });
}

}
