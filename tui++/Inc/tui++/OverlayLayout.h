#pragma once

#include <tui++/Layout.h>
#include <tui++/SizeRequirements.h>

#include <vector>

namespace tui {

class Component;

class OverlayLayout: public AbstractLayout {
  Component *const target;

  std::vector<SizeRequirements> xChildren;
  std::vector<SizeRequirements> yChildren;
  SizeRequirements xTotal;
  SizeRequirements yTotal;

public:
  OverlayLayout(Component *target) :
      target(target) {
  }

public:
  void add_layout_component(const std::shared_ptr<Component> &c, const std::any &constraints) override;
  void remove_layout_component(const std::shared_ptr<Component> &c) override;

  Dimension get_minimum_layout_size(const std::shared_ptr<const Component> &target) override;
  Dimension get_maximum_layout_size(const std::shared_ptr<const Component> &target) override;
  Dimension get_preferred_layout_size(const std::shared_ptr<const Component> &target) override;

  float get_layout_alignment_x(const std::shared_ptr<const Component> &target) const override;
  float get_layout_alignment_y(const std::shared_ptr<const Component> &target) const override;

  void invalidate_layout(const std::shared_ptr<const Component> &target) override;

  void layout(const std::shared_ptr<Component> &target) override;

private:
  void maybe_init_reqs();
};

}

