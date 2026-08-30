#pragma once

#include <tui++/border/AbstractBorder.h>

namespace tui::laf {

class MarginBorder: public AbstractBorder {
public:
  virtual Insets get_border_insets(const Component &c) const override;
};

}
