#pragma once

#include <tui++/border/AbstractBorder.h>

#include <tui++/Color.h>

#include <optional>

namespace tui::laf {

class ButtonBorder: public AbstractBorder {
public:
  ButtonBorder(std::optional<Color> const &shadow_color, std::optional<Color> const &dark_shadow_color, std::optional<Color> const &highlight_color, std::optional<Color> const &light_highlight_color) :
      shadow_color(shadow_color), dark_shadow_color(dark_shadow_color), highlight_color(highlight_color), light_highlight_color(light_highlight_color) {
  }

public:
  virtual Insets get_border_insets(Component const &c) const override;

  virtual void paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const override;

  std::optional<Color> get_shadow_color(Component const &c) const;
  std::optional<Color> get_dark_shadow_color(Component const &c) const;
  std::optional<Color> get_highlight_color(Component const &c) const;
  std::optional<Color> get_light_highlight_color(Component const &c) const;

private:
  void paint_bezel(Component const &c, Graphics &g, int x, int y, int width, int height, bool is_pressed, bool is_default) const;

private:
  std::optional<Color> shadow_color;
  std::optional<Color> dark_shadow_color;
  std::optional<Color> highlight_color;
  std::optional<Color> light_highlight_color;
};

}
