#include <tui++/lookandfeel/basic/ButtonBorder.h>

#include <tui++/AbstractButton.h>
#include <tui++/Button.h>
#include <tui++/ToggleButton.h>
#include <tui++/Component.h>
#include <tui++/Graphics.h>
#include <tui++/Shadow.h>

namespace tui::laf {

namespace {

// The shadow the button casts, or nothing. Only the button family casts one
// (see AbstractButton::get_shadow), and the border is what reserves its room
// and paints it: the shadow is a decoration of the button's box, like the
// bezel itself.
std::optional<Shadow> button_shadow(Component const &c) {
  if (auto *button = dynamic_cast<AbstractButton const *>(&c)) {
    return button->get_shadow();
  }
  return std::nullopt;
}

}

Insets ButtonBorder::get_border_insets(Component const &c) const {
  auto insets = this->shape == Shape::EDGES //
      ? Insets { 0, 1, 0, 1 } //
      : Insets { 1, 1, 1, 1 };

  // The shadow is cast by the face and falls outside it, so its room is added
  // to the face's insets: a button that casts one is that much larger, and the
  // layout leaves the shadow its cells the way it leaves room for a border.
  if (auto shadow = button_shadow(c)) {
    insets += shadow->get_insets();
  }
  return insets;
}

void ButtonBorder::paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const {
  // The face is the button minus the room reserved for its shadow; the bezel
  // is drawn on the face's own rim and the shadow in the bands outside it.
  auto shadow = button_shadow(c);
  auto shadow_insets = shadow ? shadow->get_insets() : Insets { };
  auto face_x = x + shadow_insets.left;
  auto face_y = y + shadow_insets.top;
  auto face_width = width - shadow_insets.width();
  auto face_height = height - shadow_insets.height();
  if (face_width <= 0 or face_height <= 0) {
    return; // no room to paint a face (a button squeezed into its shadow)
  }

  if (shadow) {
    shadow->paint(g, Rectangle { face_x, face_y, face_width, face_height });
  }

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

  paint_bezel(c, g, face_x, face_y, face_width, face_height, is_pressed, is_default);
}

// Paints the frame of a button: `leading` runs along the top row and the left
// column, `trailing` along the bottom row and the right column. In the EDGES
// shape only the two columns are drawn -- the rows would cost the label's own
// height. In the BOX shape the right column is painted last, so the top-right
// corner joins the dark edge, and the bottom row starts one cell in, so the
// bottom-left corner stays with the light one -- the way a raised bezel's
// corners are split between its two sides. The face behind the frame was
// filled by the component (see ComponentUI::update), so the frame only has to
// draw its edges.
void ButtonBorder::paint_frame(Graphics &g, int w, int h, std::optional<Color> const &leading, std::optional<Color> const &trailing) const {
  if (this->shape == Shape::EDGES) {
    g.set_color(leading);
    g.draw_vline(0, 0, h);
    g.set_color(trailing);
    g.draw_vline(w - 1, 0, h);
    return;
  }

  g.set_color(leading);
  g.draw_hline(0, 0, w);
  g.draw_vline(0, 1, h - 1);

  g.set_color(trailing);
  g.draw_hline(1, h - 1, w - 1);
  g.draw_vline(w - 1, 0, h - 1);
}

void ButtonBorder::paint_bezel(Component const &c, Graphics &g, int x, int y, int width, int height, bool is_pressed, bool is_default) const {
  auto h = height;
  auto w = width;

  g.translate(x, y);

  if (is_pressed and is_default) {
    // A pressed default button: one sunken frame in the darkest shadow (both
    // edges, in the EDGES shape).
    g.set_color(get_dark_shadow_color(c));
    if (this->shape == Shape::EDGES) {
      g.draw_vline(0, 0, h);
      g.draw_vline(w - 1, 0, h);
    } else {
      g.draw_rect(0, 0, w, h);
    }

  } else if (is_pressed) {
    // Sunken: the dark shadow moves to the leading edges (the top row and the
    // left column of a box, the left column of the edges).
    paint_frame(g, w, h, get_dark_shadow_color(c), get_light_highlight_color(c));

  } else if (is_default) {
    // A default button carries the strongest pair of its face's tones, the
    // way Swing gives it a border of its own.
    paint_frame(g, w, h, get_light_highlight_color(c), get_dark_shadow_color(c));

  } else {
    // Raised: the highlight along the leading edges, the shadow along the
    // trailing ones.
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
