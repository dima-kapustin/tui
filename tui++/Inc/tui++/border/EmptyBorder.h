#pragma once

#include <tui++/border/AbstractBorder.h>

namespace tui {

class EmptyBorder: public AbstractBorder {
public:
  EmptyBorder(Insets const &insets) :
      insets(insets) {
  }

  EmptyBorder(int top, int left, int bottom, int right) :
      insets(top, left, bottom, right) {
  }

public:
  virtual Insets get_border_insets(Component const &c) const override {
    return this->insets;
  }

private:
  Insets insets;
};

}
