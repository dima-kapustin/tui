#pragma once

namespace tui {

class Graphics;
class Component;

class Icon {
public:
  virtual ~Icon() {
  }

public:
  virtual void paint_icon(Component const *c, Graphics &g, int x, int y) const = 0;

  virtual int get_icon_width() const = 0;

  virtual int get_icon_height() const = 0;
};

}
