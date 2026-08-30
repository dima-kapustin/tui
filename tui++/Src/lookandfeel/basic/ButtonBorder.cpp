#include <tui++/lookandfeel/basic/ButtonBorder.h>

#include <tui++/AbstractButton.h>
#include <tui++/Button.h>
#include <tui++/Component.h>
#include <tui++/Graphics.h>

namespace tui::laf {

Insets ButtonBorder::get_border_insets(Component const &c) const {
  // leave room for the default visual
  return {2, 3, 3, 3};
}

void ButtonBorder::paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  auto is_pressed = false, is_default = false;

  if (auto *abstract_button = dynamic_cast<AbstractButton const *>(&c)) {
    auto &&model = abstract_button->get_model();
    is_pressed = model->is_pressed() and model->is_armed();

    if (auto *button = dynamic_cast<Button const *>(&c)) {
      is_default = button->is_default_button();
    }
  }

  paint_bezel(c, g, x, y, width, height, is_pressed, is_default);
}

void ButtonBorder::paint_bezel(Component const &c, Graphics &g, int x, int y, int width, int height, bool is_pressed, bool is_default) const {
  auto h = height;
  auto w = width;

  g.translate(x, y);

  if (is_pressed and is_default) {
    g.set_color(get_shadow_color(c));
    g.draw_rect(0, 0, w, h);

    g.set_color(get_dark_shadow_color(c));
    g.draw_rect(1, 1, w - 2, h - 2);

    g.set_color(get_highlight_color(c));
    g.draw_rect(2, 2, w - 4, h - 4);

  } else if (is_pressed) {
    g.set_color(get_shadow_color(c));
    g.draw_hline(0, 0, w);
    g.draw_vline(0, 1, h - 2);

    g.set_color(get_dark_shadow_color(c));
    g.draw_hline(1, 1, w - 2);
    g.draw_vline(1, 2, h - 3);

    g.set_color(get_highlight_color(c));
    g.draw_hline(0, h - 1, w);
    g.draw_vline(w - 1, 0, h - 1);

    g.set_color(get_light_highlight_color(c));
    g.draw_hline(1, h - 2, w - 2);
    g.draw_vline(w - 2, 1, h - 3);

  } else if (is_default) {
    g.set_color(get_light_highlight_color(c));
    g.draw_rect(0, 0, w, h);

    g.set_color(get_highlight_color(c));
    g.draw_rect(1, 1, w - 2, h - 2);

    g.set_color(get_shadow_color(c));
    g.draw_vline(w - 2, h - 2, 1);

    g.set_color(get_dark_shadow_color(c));
    g.draw_vline(w - 1, h - 1, 1);

  } else {
    g.set_color(get_highlight_color(c));
    g.draw_hline(0, 0, w - 1);
    g.draw_vline(0, 1, h - 3);

    g.set_color(get_light_highlight_color(c));
    g.draw_hline(1, 1, w - 3);
    g.draw_vline(1, 2, h - 4);

    g.set_color(get_shadow_color(c));
    g.draw_hline(0, h - 1, w);
    g.draw_vline(w - 1, 0, h - 1);

    g.set_color(get_dark_shadow_color(c));
    g.draw_hline(1, h - 2, w - 2);
    g.draw_vline(w - 2, 1, h - 3);
  }

  g.translate(-x, -y);
}

std::optional<Color> ButtonBorder::get_shadow_color(Component const &c) const {
  return this->shadow_color ? this->shadow_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.darker();
  });
}

std::optional<Color> ButtonBorder::get_dark_shadow_color(Component const &c) const {
  return this->dark_shadow_color ? this->dark_shadow_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.darker().darker();
  });
}

std::optional<Color> ButtonBorder::get_highlight_color(Component const &c) const {
  return this->highlight_color ? this->highlight_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.brighter();
  });
}

std::optional<Color> ButtonBorder::get_light_highlight_color(Component const &c) const {
  return this->light_highlight_color ? this->light_highlight_color : c.get_background_color().transform([](auto const &background_color) {
    return background_color.brighter().brighter();
  });
}

}
