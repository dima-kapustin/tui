#include <tui++/BoxLayout.h>
#include <tui++/Component.h>

#include <cassert>

namespace tui {

static BoxLayout::Axis resolve_axis(BoxLayout::Axis axis, ComponentOrientation const &o) {
  if (axis == BoxLayout::Axis::LINE) {
    return o.is_horizontal() ? BoxLayout::Axis::X : BoxLayout::Axis::Y;
  } else if (axis == BoxLayout::Axis::PAGE) {
    return o.is_horizontal() ? BoxLayout::Axis::Y : BoxLayout::Axis::X;
  } else {
    return axis;
  }
}

void BoxLayout::add_layout_component(const std::shared_ptr<Component> &c, const Constraints &constraints) {
  invalidate_layout(c->get_parent());
}

void BoxLayout::remove_layout_component(const std::shared_ptr<Component> &c) {
  invalidate_layout(c->get_parent());
}

Dimension BoxLayout::get_minimum_layout_size(const std::shared_ptr<const Component> &target) {
  assert(this->target == target.get());
  maybe_init_reqs();

  auto size = Dimension { this->xTotal.minimum, this->yTotal.minimum };
  auto insets = target->get_insets();
  size.width += insets.left + insets.right;
  size.height += insets.top + insets.bottom;
  return size;
}

Dimension BoxLayout::get_maximum_layout_size(const std::shared_ptr<const Component> &target) {
  assert(this->target == target.get());
  maybe_init_reqs();

  auto size = Dimension { this->xTotal.maximum, this->yTotal.maximum };
  auto insets = target->get_insets();
  size.width += insets.left + insets.right;
  size.height += insets.top + insets.bottom;
  return size;
}

Dimension BoxLayout::get_preferred_layout_size(const std::shared_ptr<const Component> &target) {
  assert(this->target == target.get());
  maybe_init_reqs();

  auto size = Dimension { this->xTotal.preferred, this->yTotal.preferred };
  auto insets = target->get_insets();
  size.width += insets.left + insets.right;
  size.height += insets.top + insets.bottom;
  return size;
}

float BoxLayout::get_layout_alignment_x(const std::shared_ptr<const Component> &target) const {
  assert(this->target == target.get());
  const_cast<BoxLayout*>(this)->maybe_init_reqs();
  return this->xTotal.alignment;
}

float BoxLayout::get_layout_alignment_y(const std::shared_ptr<const Component> &target) const {
  assert(this->target == target.get());
  const_cast<BoxLayout*>(this)->maybe_init_reqs();
  return this->yTotal.alignment;
}

void BoxLayout::invalidate_layout(const std::shared_ptr<const Component> &target) {
  assert(this->target == target.get());
  this->xChildren.clear();
  this->yChildren.clear();
}

void BoxLayout::layout(const std::shared_ptr<Component> &target) {
  assert(target.get() == this->target);
  maybe_init_reqs();

  auto nChildren = target->get_component_count();
  auto xOffsets = std::vector<int>(nChildren);
  auto xSpans = std::vector<int>(nChildren);
  auto yOffsets = std::vector<int>(nChildren);
  auto ySpans = std::vector<int>(nChildren);

  auto size = target->get_size();
  auto insets = target->get_insets();
  size.width -= insets.left + insets.right;
  size.height -= insets.top + insets.bottom;

  auto o = target->get_component_orientation();
  auto absolute_axis = resolve_axis(this->axis, o);
  auto ltr = (absolute_axis != this->axis) ? o.is_left_to_right() : true;

  if (absolute_axis == Axis::X) {
    SizeRequirements::calculate_tiled_positions(size.width, this->xChildren, xOffsets, xSpans, ltr);
    SizeRequirements::calculate_aligned_positions(size.height, this->yTotal, this->yChildren, yOffsets, ySpans);
  } else {
    SizeRequirements::calculate_aligned_positions(size.width, this->xTotal, this->xChildren, xOffsets, xSpans, ltr);
    SizeRequirements::calculate_tiled_positions(size.height, this->yChildren, yOffsets, ySpans);
  }

  for (auto i = 0U; i != nChildren; i++) {
    target->get_component(i)->set_bounds(insets.left + xOffsets[i], insets.top + yOffsets[i], xSpans[i], ySpans[i]);
  }

}

void BoxLayout::maybe_init_reqs() {
  if (this->xChildren.empty() or this->yChildren.empty()) {
    auto n = this->target->get_component_count();
    this->xChildren.reserve(n);
    this->yChildren.reserve(n);
    for (auto &&c : target->get_components()) {
      if (c->is_visible()) {
        auto min = c->get_minimum_size();
        auto typ = c->get_preferred_size();
        auto max = c->get_maximum_size();
        this->xChildren.emplace_back(min.width, typ.width, max.width, c->get_alignment_x());
        this->yChildren.emplace_back(min.height, typ.height, max.height, c->get_alignment_y());
      } else {
        this->xChildren.emplace_back(0, 0, 0, c->get_alignment_x());
        this->yChildren.emplace_back(0, 0, 0, c->get_alignment_y());
      }
    }

    if (auto axis = resolve_axis(this->axis, this->target->get_component_orientation()); axis == Axis::X) {
      this->xTotal = SizeRequirements::get_tiled_size_requirements(this->xChildren);
      this->yTotal = SizeRequirements::get_aligned_size_requirements(this->yChildren);
    } else {
      this->xTotal = SizeRequirements::get_aligned_size_requirements(this->xChildren);
      this->yTotal = SizeRequirements::get_tiled_size_requirements(this->yChildren);
    }
  }
}
}
