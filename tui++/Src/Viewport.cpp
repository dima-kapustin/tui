#include <tui++/Viewport.h>
#include <tui++/Component.h>
#include <tui++/Graphics.h>

namespace tui {

Viewport::Viewport() {
  set_background_color(Color { 12, 12, 16 });
}

void Viewport::paint(Graphics &g) {
  // Fill the viewport background first (the view may not cover it, e.g. when
  // the content is shorter than the viewport); the view child itself is
  // painted by paint_children, clipped to this viewport.
  auto bg = get_background_color();
  if (bg) {
    g.set_background_color(bg);
    g.fill_rect(0, 0, get_width(), get_height());
  }
  base::paint(g);
}

void Viewport::set_view(const std::shared_ptr<Component> &view) {
  if (this->view) {
    remove(this->view);
  }
  this->view = view;
  if (view) {
    add(view);
    this->view_size = view->get_preferred_size();
    if (this->view_size.width <= 0) {
      this->view_size.width = get_width();
    }
    if (this->view_size.height <= 0) {
      this->view_size.height = get_height();
    }
  }
  clamp_view_position();
  place_view();
}

void Viewport::place_view() {
  if (this->view) {
    // The child's location is the negated view position, so its content flows
    // upward as the viewport scrolls down.
    this->view->set_bounds(-this->view_position.x, -this->view_position.y, this->view_size.width, this->view_size.height);
  }
}

void Viewport::clamp_view_position() {
  auto max_x = std::max(0, this->view_size.width - get_width());
  auto max_y = std::max(0, this->view_size.height - get_height());
  this->view_position.x = std::clamp(this->view_position.x, 0, max_x);
  this->view_position.y = std::clamp(this->view_position.y, 0, max_y);
}

void Viewport::set_view_position(int x, int y) {
  if (x == this->view_position.x and y == this->view_position.y) {
    return;
  }
  this->view_position = { x, y };
  clamp_view_position();
  place_view();
  repaint();
  notify_position_changed();
}

void Viewport::set_view_size(int width, int height) {
  width = std::max(0, width);
  height = std::max(0, height);
  if (width == this->view_size.width and height == this->view_size.height) {
    return;
  }
  this->view_size = { width, height };
  clamp_view_position();
  place_view();
  repaint();
  if (this->on_view_size_changed) {
    this->on_view_size_changed();
  }
}

void Viewport::scroll_rect_to_visible(Rectangle const &rect) {
  auto x = this->view_position.x;
  auto y = this->view_position.y;
  if (rect.y < y) {
    y = rect.y;
  } else if (rect.y + rect.height > y + get_height()) {
    y = rect.y + rect.height - get_height();
  }
  if (rect.x < x) {
    x = rect.x;
  } else if (rect.x + rect.width > x + get_width()) {
    x = rect.x + rect.width - get_width();
  }
  if (x != this->view_position.x or y != this->view_position.y) {
    set_view_position(x, y);
  }
}

}
