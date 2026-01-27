#pragma once

#include <tui++/border/AbstractBorder.h>

#include <tui++/Color.h>

#include <optional>

namespace tui {

class BevelBorder: public AbstractBorder {
public:
  enum BevelType {
    RAISED,
    LOWERED
  };

public:
  BevelBorder(BevelType bevel_type) :
      bevel_type(bevel_type) {
  }

  BevelBorder(BevelType bevel_type, Color highlight_color, Color shadow_color) {
    BevelBorder(bevel_type, highlight_color.brighter(), highlight_color, shadow_color, shadow_color.brighter());
  }

  BevelBorder(BevelType bevel_type, std::optional<Color> highlight_outer_color, std::optional<Color> highlight_inner_color, std::optional<Color> shadow_outer_color, std::optional<Color> shadow_inner_color) :
      bevel_type(bevel_type), highlight_outer_color(highlight_outer_color), highlight_inner_color(highlight_inner_color), shadow_outer_color(shadow_outer_color), shadow_inner_color(shadow_inner_color) {
  }

public:
  virtual Insets get_border_insets(Component const &c) const override;

  virtual void paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const override;

  virtual bool is_border_opaque() const override {
    return true;
  }

  std::optional<Color> get_highlight_outer_color(Component const &c) const;
  std::optional<Color> get_highlight_inner_color(Component const &c) const;
  std::optional<Color> get_shadow_inner_color(Component const &c) const;
  std::optional<Color> get_shadow_outer_color(Component const &c) const;

private:
  void paint_raised_bevel(Component const &c, Graphics &g, int x, int y, int width, int height) const;
  void paint_lowered_bevel(Component const &c, Graphics &g, int x, int y, int width, int height) const;

private:
  BevelType bevel_type;
  std::optional<Color> highlight_outer_color;
  std::optional<Color> highlight_inner_color;
  std::optional<Color> shadow_outer_color;
  std::optional<Color> shadow_inner_color;
};

}
