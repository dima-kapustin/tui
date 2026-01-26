#pragma once

#include <tui++/border/AbstractBorder.h>

#include <tui++/Color.h>

#include <optional>

namespace tui {

class EtchedBorder: public AbstractBorder {
public:
  enum EtchType {
    RAISED,
    LOWERED
  };

  EtchedBorder() :
      EtchedBorder(EtchType::LOWERED) {
  }

  EtchedBorder(EtchType etch_type) :
      etch_type(etch_type) {
  }

  EtchedBorder(std::optional<Color> const &highlight_color, std::optional<Color> const &shadow_color) :
      EtchedBorder(EtchType::LOWERED, highlight_color, shadow_color) {
  }

  EtchedBorder(EtchType etch_type, std::optional<Color> const &highlight_color, std::optional<Color> const &shadow_color) :
      etch_type(etch_type), highlight_color(highlight_color), shadow_color(shadow_color) {
  }

public:
  virtual void paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const override;

  virtual bool is_border_opaque() const override {
    return true;
  }

  std::optional<Color> get_highlight_color(Component const &c) const;
  std::optional<Color> get_shadow_color(Component const &c) const;

private:
  void paint_border_highlight(Graphics &g, std::optional<Color> const &c, int w, int h, int stkWidth) const;
  void paint_border_shadow(Graphics &g, std::optional<Color> const &c, int w, int h, int stkWidth) const;

private:
  EtchType etch_type;
  std::optional<Color> highlight_color;
  std::optional<Color> shadow_color;
};

}
