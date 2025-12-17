#pragma once

#include <tui++/Point.h>
#include <tui++/Dimension.h>

#include <memory>

namespace tui {
class Graphics;
class Component;
}

namespace tui::laf {

struct ComponentUI {
  virtual void install_ui(std::shared_ptr<Component> const &c) = 0;
  virtual void uninstall_ui(std::shared_ptr<Component> const &c) = 0;

  virtual void paint(std::shared_ptr<Graphics> const &g, std::shared_ptr<const Component> const &c) const = 0;

  virtual void update(std::shared_ptr<Graphics> const &g, std::shared_ptr<const Component> const &c) const;

  virtual Dimension get_preferred_size(std::shared_ptr<const Component> const &c) const {
    return Dimension::zero();
  }

  virtual Dimension get_minimum_size(std::shared_ptr<const Component> const &c) const {
    return get_preferred_size(c);
  }

  virtual Dimension get_maximum_size(std::shared_ptr<const Component> const &c) const {
    return get_preferred_size(c);
  }

  virtual bool contains(std::shared_ptr<const Component> const &c, int x, int y) const;

  bool contains(std::shared_ptr<const Component> const &c, Point const &p) const {
    return contains(c, p.x, p.y);
  }
};

}
