#pragma once

#include <tui++/border/AbstractBorder.h>

namespace tui {

class CompoundBorder: public AbstractBorder {
public:
  CompoundBorder(std::shared_ptr<Border> const &outside_border, std::shared_ptr<Border> const &inside_border) :
      outside_border(outside_border), inside_border(inside_border) {
  }

public:
  virtual Insets get_border_insets(Component const &c) const override;

  virtual void paint_border(Component const &c, Graphics &g, int x, int y, int width, int height) const;

  virtual bool is_border_opaque() const override;

private:
  std::shared_ptr<Border> outside_border;
  std::shared_ptr<Border> inside_border;
};

}
