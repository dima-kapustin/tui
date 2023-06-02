#pragma once

#include <any>
#include <memory>

namespace tui {

class Component;

class Layout {
public:
  virtual ~Layout() {
  }

  virtual void add_layout_component(const std::shared_ptr<Component> &component, const std::any &constraints) = 0;

  virtual void remove_layout_component(const std::shared_ptr<Component> &component) = 0;

  virtual void layout(const std::shared_ptr<Component> &component) = 0;
};

}
