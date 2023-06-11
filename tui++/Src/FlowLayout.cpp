#include <tui++/FlowLayout.h>
#include <tui++/Component.h>

namespace tui {

Dimension FlowLayout::get_minimum_layout_size(const std::shared_ptr<const Component> &target) {
  auto lock = target->get_tree_lock();

  auto target_size = Dimension { };
  auto use_hgap = true;

  for (auto &&c : *target) {
    if (c->is_visible()) {
      auto child_size = c->get_minimum_size();
      target_size.height = std::max(target_size.height, child_size.height);
      if (use_hgap) {
        use_hgap = false;
      } else {
        target_size.width += this->hgap;
      }
      target_size.width += child_size.width;
    }
  }

  auto insets = target->get_insets();
  target_size.width += insets.left + insets.right + this->hgap * 2;
  target_size.height += insets.top + insets.bottom + this->vgap * 2;
  return target_size;
}

Dimension FlowLayout::get_preferred_layout_size(const std::shared_ptr<const Component> &target) {
  auto lock = target->get_tree_lock();

  auto target_size = Dimension { };
  bool use_hgap = true;

  for (auto &&c : *target) {
    if (c->is_visible()) {
      auto d = c->get_preferred_size();
      target_size.height = std::max(target_size.height, d.height);
      if (use_hgap) {
        use_hgap = false;
      } else {
        target_size.width += this->hgap;
      }
      target_size.width += d.width;
    }
  }
  auto insets = target->get_insets();
  target_size.width += insets.left + insets.right + this->hgap * 2;
  target_size.height += insets.top + insets.bottom + this->vgap * 2;
  return target_size;

}

void FlowLayout::layout(const std::shared_ptr<Component> &target) {
  auto lock = target->get_tree_lock();

  auto insets = target->get_insets();
  auto max_width = target->get_width() - (insets.left + insets.right + this->hgap * 2);
  auto x = 0, y = insets.top + this->vgap;
  auto rowh = 0;
  auto start = size_t { 0 };

  auto ltr = target->get_component_orientation().is_left_to_right();

  auto index = size_t { 0 };
  for (auto &&c : *target) {
    if (c->is_visible()) {
      auto d = c->get_preferred_size();
      c->set_size(d.width, d.height);

      if ((x == 0) || ((x + d.width) <= max_width)) {
        if (x > 0) {
          x += this->hgap;
        }
        x += d.width;
        rowh = std::max(rowh, d.height);
      } else {
        rowh = move_components(*target, insets.left + this->hgap, y, max_width - x, rowh, start, index, ltr);
        x = d.width;
        y += this->vgap + rowh;
        rowh = d.height;
        start = index;
      }
    }
    index += 1;
  }
  move_components(*target, insets.left + this->hgap, y, max_width - x, rowh, start, target->get_component_count(), ltr);
}

int FlowLayout::move_components(Component &target, int x, int y, int width, int height, size_t row_Start, size_t row_end, bool ltr) {
  switch (this->align) {
  case LEFT:
    x += ltr ? 0 : width;
    break;
  case CENTER:
    x += width / 2;
    break;
  case RIGHT:
    x += ltr ? width : 0;
    break;
  case LEADING:
    break;
  case TRAILING:
    x += width;
    break;
  }
  for (size_t i = row_Start; i < row_end; i++) {
    auto c = target.get_component(i);
    if (c->is_visible()) {
      int cy = y + (height - c->get_height()) / 2;
      if (ltr) {
        c->set_location(x, cy);
      } else {
        c->set_location(target.get_width() - x - c->get_width(), cy);
      }
      x += c->get_width() + this->hgap;
    }
  }
  return height;
}

}
