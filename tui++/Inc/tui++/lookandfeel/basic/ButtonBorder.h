#pragma once

#include <tui++/border/AbstractBorder.h>

#include <tui++/Color.h>

#include <optional>

namespace tui::laf {

class ButtonBorder: public AbstractBorder {
public:
  // How much of a frame the bezel draws. A character cell is as tall as the
  // button's label, so the text look has room only for the two vertical
  // edges beside the label (EDGES): a top and a bottom edge would grow a
  // one-row button to three rows. The pixel backends keep the full BOX --
  // their 1px lines cost no row of content.
  enum class Shape {
    EDGES,
    BOX
  };

  ButtonBorder(std::optional<Color> const &shadow_color, std::optional<Color> const &dark_shadow_color, std::optional<Color> const &highlight_color, std::optional<Color> const &light_highlight_color, Shape shape = Shape::BOX) :
      shadow_color(shadow_color), dark_shadow_color(dark_shadow_color), highlight_color(highlight_color), light_highlight_color(light_highlight_color), shape(shape) {
  }

public:
  virtual Insets get_border_insets(Component const &c) const override;

  virtual void paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const override;

  std::optional<Color> get_shadow_color(Component const &c) const;
  std::optional<Color> get_dark_shadow_color(Component const &c) const;
  std::optional<Color> get_highlight_color(Component const &c) const;
  std::optional<Color> get_light_highlight_color(Component const &c) const;

private:
  // The frame of a state: the color of its leading edges (the top row and the
  // left column of a BOX, the left edge of EDGES) and the color of its
  // trailing ones.
  void paint_frame(Graphics &g, int w, int h, std::optional<Color> const &leading, std::optional<Color> const &trailing) const;

  void paint_bezel(Component const &c, Graphics &g, int x, int y, int width, int height, bool is_pressed, bool is_default) const;

private:
  std::optional<Color> shadow_color;
  std::optional<Color> dark_shadow_color;
  std::optional<Color> highlight_color;
  std::optional<Color> light_highlight_color;
  Shape shape = Shape::BOX;
};

}
