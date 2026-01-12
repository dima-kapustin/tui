#pragma once

#include <tui++/Color.h>
#include <tui++/Border.h>
#include <tui++/Stroke.h>

#include <memory>
#include <optional>

namespace tui {

class LineBorder: public Border {
  Stroke const stroke;
  Color const line_color;
  std::optional<Color> background_color;

public:
  LineBorder(Stroke const &stroke, Color const &line_color, std::optional<Color> const &background_color = { }) :
      stroke(stroke), line_color(line_color), background_color(background_color) {
  }

  virtual Insets get_border_insets(const Component &c) const override;

  virtual void paint_border(const Component &c, Graphics &g, int x, int y, int width, int height) const override;
};

}
