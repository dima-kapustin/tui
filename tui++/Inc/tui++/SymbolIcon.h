#pragma once

#include <tui++/Char.h>
#include <tui++/Icon.h>

namespace tui {

class SymbolIcon: public Icon {
  Char const symbol;

public:
  SymbolIcon(Char const &symbol) :
      symbol(symbol) {
  }

public:
  virtual void paint_icon(Component const *c, Graphics &g, int x, int y) const override;

  virtual int get_icon_width() const override;

  virtual int get_icon_height() const override;
};

}
