#pragma once

#include <any>
#include <memory>

#include <tui++/Dimension.h>

namespace tui {

class Component;

class Layout {
public:
  virtual ~Layout() {
  }

  virtual void add_layout_component(const std::shared_ptr<Component> &component, const std::any &constraints) = 0;

  virtual void remove_layout_component(const std::shared_ptr<Component> &component) = 0;

  virtual void layout(const std::shared_ptr<Component> &component) = 0;

  virtual Dimension get_minimum_layout_size(const std::shared_ptr<const Component> &component) = 0;

  virtual Dimension get_preferred_layout_size(const std::shared_ptr<const Component> &component) = 0;

  /**
   * Returns the alignment along the x axis. This specifies how the component would like to be aligned relative to other components. The
   * value should be a number between 0 and 1 where 0 represents alignment along the origin, 1 is aligned the furthest away from the
   * origin, 0.5 is centered, etc.
   */
  virtual float get_layout_alignment_x(const std::shared_ptr<const Component> &component) = 0;

  /**
   * Returns the alignment along the y axis. This specifies how the component would like to be aligned relative to other components. The
   * value should be a number between 0 and 1 where 0 represents alignment along the origin, 1 is aligned the furthest away from the
   * origin, 0.5 is centered, etc.
   */
  virtual float get_layout_alignment_y(const std::shared_ptr<const Component> &component) = 0;

  /**
   * Invalidates the layout, indicating that if this layout manager has cached information it should be discarded.
   */
  void invalidate_layout(const std::shared_ptr<const Component> &component);
};

}
