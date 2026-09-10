#include <tui++/lookandfeel/basic/ButtonBorder.h>

#include <tui++/AbstractButton.h>
#include <tui++/Button.h>
#include <tui++/ToggleButton.h>
#include <tui++/Component.h>
#include <tui++/Graphics.h>

namespace tui::laf {

Insets ButtonBorder::get_border_insets(Component const &c) const {
  // The bezel is one cell wide on every side: the label sits in a one-cell
  // frame, the way Swing's button border is one pixel thick. The two-pixel
  // bevels of the platform look-and-feels have no room in a character cell,
  // and a taller frame would push the button's label out of line with the
  // text fields and combo boxes beside it.
  return { 1, 1, 1, 1 };
}

void ButtonBorder::paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  auto is_pressed = false, is_default = false;

  if (auto *abstract_button = dynamic_cast<AbstractButton const *>(&c)) {
    auto &&model = abstract_button->get_model();
    // A selected toggleable button (ToggleButton and its CheckBox/RadioButton
    // subclasses) keeps the sunken "pressed" look after the click, the way a
    // Swing toggle shows itself selected.
    is_pressed = (model->is_pressed() and model->is_armed()) or (dynamic_cast<ToggleButton const*>(&c) and model->is_selected());

    if (auto *button = dynamic_cast<Button const *>(&c)) {
      is_default = button->is_default_button();
    }
  }

  paint_bezel(c, g, x, y, width, height, is_pressed, is_default);
}

// Paints the one-cell frame of a button: `top_left` along the top row and the
// left column, `bottom_right` along the bottom row and the right column. The
// right column is painted last, so the top-right corner joins the dark edge,
// and the bottom row starts one cell in, so the bottom-left corner stays with
// the light one -- the way a raised bezel's corners are split between its two
// sides. The face behind the frame was filled by the component (see
// ComponentUI::update), so the frame only has to draw its edges.
void ButtonBorder::paint_frame(Graphics &g, int w, int h, std::optional<Color> const &top_left, std::optional<Color> const &bottom_right) const {
  g.set_color(top_left);
  g.draw_hline(0, 0, w);
  g.draw_vline(0, 1, h - 1);

  g.set_color(bottom_right);
  g.draw_hline(1, h - 1, w - 1);
  g.draw_vline(w - 1, 0, h - 1);
}

void ButtonBorder::paint_bezel(Component const &c, Graphics &g, int x, int y, int width, int height, bool is_pressed, bool is_default) const {
  auto h = height;
  auto w = width;

  g.translate(x, y);

  if (is_pressed and is_default) {
    // A pressed default button: one sunken frame in the darkest shadow.
    g.set_color(get_dark_shadow_color(c));
    g.draw_rect(0, 0, w, h);

  } else if (is_pressed) {
    // Sunken: the dark shadow moves to the top and left.
    paint_frame(g, w, h, get_dark_shadow_color(c), get_light_highlight_color(c));

  } else if (is_default) {
    // A default button carries the strongest pair of its face's tones, the
    // way Swing gives it a border of its own.
    paint_frame(g, w, h, get_light_highlight_color(c), get_dark_shadow_color(c));

  } else {
    // Raised: the highlight along the top and left, the shadow along the
    // bottom and right.
    paint_frame(g, w, h, get_highlight_color(c), get_shadow_color(c));
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
