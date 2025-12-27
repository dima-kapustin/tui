#pragma once

#include <tui++/Point.h>
#include <tui++/Dimension.h>

#include <memory>

namespace tui {
class Graphics;
class Component;
}

namespace tui::laf {

class ComponentUI {
public:
  virtual ~ComponentUI();

public:
  virtual void install_ui(std::shared_ptr<Component> const &c);
  virtual void uninstall_ui(std::shared_ptr<Component> const &c);

  virtual void update(Graphics &g, std::shared_ptr<const Component> const &c) const;

  virtual Dimension get_preferred_size(std::shared_ptr<const Component> const &c) const;
  virtual Dimension get_minimum_size(std::shared_ptr<const Component> const &c) const;
  virtual Dimension get_maximum_size(std::shared_ptr<const Component> const &c) const;

  virtual bool contains(std::shared_ptr<const Component> const &c, int x, int y) const;

  bool contains(std::shared_ptr<const Component> const &c, Point const &p) const {
    return contains(c, p.x, p.y);
  }

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const;
};

}
