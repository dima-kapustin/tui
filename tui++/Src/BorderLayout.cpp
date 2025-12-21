#include <tui++/BorderLayout.h>

#include <tui++/Component.h>
#include <tui++/ComponentOrientation.h>

namespace tui {

void BorderLayout::add_layout_component(const std::shared_ptr<Component> &c, const Constraints &constraints) {
  if (auto name = std::get_if<std::string_view>(&constraints)) {
    if (*name == CENTER) {
      this->center = c;
    } else if (*name == NORTH) {
      this->north = c;
    } else if (*name == WEST) {
      this->west = c;
    } else if (*name == SOUTH) {
      this->south = c;
    } else if (*name == EAST) {
      this->east = c;
    } else if (*name == PAGE_START) {
      this->first_line = c;
    } else if (*name == PAGE_END) {
      this->last_line = c;
    } else if (*name == LINE_START) {
      this->first_item = c;
    } else if (*name == LINE_END) {
      this->last_item = c;
    } else {
      throw std::runtime_error("Cannot add to layout: unknown constraint: " + std::string(*name));
    }
  } else if (std::holds_alternative<std::monostate>(constraints)) {
    this->center = c;
  } else {
    throw std::runtime_error("Cannot add to layout: constraint must be a string_view (or null)");
  }
}

void BorderLayout::remove_layout_component(const std::shared_ptr<Component> &c) {
  c->with_tree_locked([c, this] {
    if (c == this->center) {
      this->center = nullptr;
    } else if (c == this->north) {
      this->north = nullptr;
    } else if (c == this->south) {
      this->south = nullptr;
    } else if (c == this->east) {
      this->east = nullptr;
    } else if (c == this->west) {
      this->west = nullptr;
    }

    if (c == this->first_line) {
      this->first_line = nullptr;
    } else if (c == this->last_line) {
      this->last_line = nullptr;
    } else if (c == this->first_item) {
      this->first_item = nullptr;
    } else if (c == this->last_item) {
      this->last_item = nullptr;
    }
  });
}

std::shared_ptr<Component> BorderLayout::get_layout_component(const Constraints &constraints) {
  if (auto name = std::get_if<std::string_view>(&constraints)) {
    if (CENTER == *name) {
      return this->center;
    } else if (NORTH == *name) {
      return this->north;
    } else if (SOUTH == *name) {
      return this->south;
    } else if (WEST == *name) {
      return this->west;
    } else if (EAST == *name) {
      return this->east;
    } else if (PAGE_START == *name) {
      return this->first_line;
    } else if (PAGE_END == *name) {
      return this->last_line;
    } else if (LINE_START == *name) {
      return this->first_item;
    } else if (LINE_END == *name) {
      return this->last_item;
    } else {
      throw std::runtime_error("Cannot get component: unknown constraint: " + std::string(*name));
    }
  } else if (std::holds_alternative<std::monostate>(constraints)) {
    return this->center;
  } else {
    throw std::runtime_error("Cannot get component: constraint must be a string_view");
  }
}

std::shared_ptr<Component> BorderLayout::get_north_component() noexcept (true) {
  return this->first_line ? this->first_line : this->north;
}

std::shared_ptr<Component> BorderLayout::get_south_component() noexcept (true) {
  return this->last_line ? this->last_line : this->south;
}

std::shared_ptr<Component> BorderLayout::get_east_component(const ComponentOrientation &orientation) noexcept (true) {
  if (auto east = orientation.is_left_to_right() ? this->last_item : this->first_item) {
    return east;
  }
  return this->east;
}

std::shared_ptr<Component> BorderLayout::get_west_component(const ComponentOrientation &orientation) noexcept (true) {
  if (auto west = orientation.is_left_to_right() ? this->first_item : this->last_item) {
    return west;
  }
  return this->west;
}

std::shared_ptr<Component> BorderLayout::get_layout_component(const ComponentOrientation &orientation, const std::string_view &constraints) {
  if (NORTH == constraints) {
    return get_north_component();
  } else if (SOUTH == constraints) {
    return get_south_component();
  } else if (WEST == constraints) {
    return get_west_component(orientation);
  } else if (EAST == constraints) {
    return get_east_component(orientation);
  } else if (CENTER == constraints) {
    return this->center;
  } else {
    throw std::runtime_error("Cannot get component: invalid constraint: " + std::string(constraints));
  }
}

std::shared_ptr<Component> BorderLayout::get_layout_component(const ComponentOrientation &orientation, const Constraints &constraints) {
  if (auto name = std::get_if<std::string_view>(&constraints)) {
    return get_layout_component(orientation, *name);
  } else {
    throw std::runtime_error("Cannot get component: constraint must be a string_view");
  }
}

std::optional<std::string_view> BorderLayout::get_constraints(const std::shared_ptr<Component> &target) {
  if (not target) {
    return {};
  }
  if (target == this->center) {
    return CENTER;
  } else if (target == this->north) {
    return NORTH;
  } else if (target == this->south) {
    return SOUTH;
  } else if (target == this->west) {
    return WEST;
  } else if (target == this->east) {
    return EAST;
  } else if (target == this->first_line) {
    return PAGE_START;
  } else if (target == this->last_line) {
    return PAGE_END;
  } else if (target == this->first_item) {
    return LINE_START;
  } else if (target == this->last_item) {
    return LINE_END;
  }
  return {};
}

Dimension BorderLayout::get_minimum_layout_size(const std::shared_ptr<const Component> &target) {
  return target->with_tree_locked([target, this] {
    auto dim = Dimension { };

    auto orientation = target->get_component_orientation();

    if (auto c = get_east_component(orientation); c and c->is_visible()) {
      auto d = c->get_minimum_size();
      dim.width += d.width + this->hgap;
      dim.height = std::max(d.height, dim.height);
    }

    if (auto c = get_west_component(orientation); c and c->is_visible()) {
      auto d = c->get_minimum_size();
      dim.width += d.width + this->hgap;
      dim.height = std::max(d.height, dim.height);
    }

    if (auto c = this->center; c and c->is_visible()) {
      auto d = c->get_minimum_size();
      dim.width += d.width;
      dim.height = std::max(d.height, dim.height);
    }

    if (auto c = get_north_component(); c and c->is_visible()) {
      auto d = c->get_minimum_size();
      dim.width = std::max(d.width, dim.width);
      dim.height += d.height + this->vgap;
    }

    if (auto c = get_south_component(); c and c->is_visible()) {
      auto d = c->get_minimum_size();
      dim.width = std::max(d.width, dim.width);
      dim.height += d.height + this->vgap;
    }

    auto insets = target->get_insets();
    dim.width += insets.left + insets.right;
    dim.height += insets.top + insets.bottom;
    return dim;
  });
}

Dimension BorderLayout::get_maximum_layout_size(const std::shared_ptr<const Component> &target) {
  return Dimension::max();
}

Dimension BorderLayout::get_preferred_layout_size(const std::shared_ptr<const Component> &target) {
  return target->with_tree_locked([target, this] {
    auto dim = Dimension { };

    auto orientation = target->get_component_orientation();

    if (auto c = get_east_component(orientation); c and c->is_visible()) {
      auto d = c->get_preferred_size();
      dim.width += d.width + this->hgap;
      dim.height = std::max(d.height, dim.height);
    }

    if (auto c = get_west_component(orientation); c and c->is_visible()) {
      auto d = c->get_preferred_size();
      dim.width += d.width + this->hgap;
      dim.height = std::max(d.height, dim.height);
    }

    if (auto c = this->center; c and c->is_visible()) {
      auto d = c->get_preferred_size();
      dim.width += d.width;
      dim.height = std::max(d.height, dim.height);
    }

    if (auto c = get_north_component(); c and c->is_visible()) {
      auto d = c->get_preferred_size();
      dim.width = std::max(d.width, dim.width);
      dim.height += d.height + this->vgap;
    }

    if (auto c = get_south_component(); c and c->is_visible()) {
      auto d = c->get_preferred_size();
      dim.width = std::max(d.width, dim.width);
      dim.height += d.height + this->vgap;
    }

    auto insets = target->get_insets();
    dim.width += insets.left + insets.right;
    dim.height += insets.top + insets.bottom;
    return dim;
  });
}

void BorderLayout::layout(const std::shared_ptr<Component> &target) {
  target->with_tree_locked([target, this] {
    auto insets = target->get_insets();
    auto top = insets.top;
    auto bottom = target->get_height() - insets.bottom;
    auto left = insets.left;
    auto right = target->get_width() - insets.right;

    auto orientation = target->get_component_orientation();

    if (auto c = get_north_component(); c and c->is_visible()) {
      c->set_size(right - left, c->get_height());
      auto d = c->get_preferred_size();
      c->set_bounds(left, top, right - left, d.height);
      top += d.height + this->vgap;
    }

    if (auto c = get_south_component(); c and c->is_visible()) {
      c->set_size(right - left, c->get_height());
      auto d = c->get_preferred_size();
      c->set_bounds(left, bottom - d.height, right - left, d.height);
      bottom -= d.height + this->vgap;
    }

    if (auto c = get_east_component(orientation); c and c->is_visible()) {
      c->set_size(c->get_width(), bottom - top);
      auto d = c->get_preferred_size();
      c->set_bounds(right - d.width, top, d.width, bottom - top);
      right -= d.width + this->hgap;
    }

    if (auto c = get_west_component(orientation); c and c->is_visible()) {
      c->set_size(c->get_width(), bottom - top);
      auto d = c->get_preferred_size();
      c->set_bounds(left, top, d.width, bottom - top);
      left += d.width + this->hgap;
    }

    if (auto c = this->center; c and c->is_visible()) {
      c->set_bounds(left, top, right - left, bottom - top);
    }
  });
}

}
