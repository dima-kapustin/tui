#pragma once

// Viewport - Swing's JViewport: the window through which a (larger) "view"
// component is seen. The view is a regular child positioned at
// (-view_position); painting, mouse hit-testing and repaints flow through the
// normal component machinery, so only the intersection with the viewport is
// ever drawn. Scrolling repositions the child and repaints.

#include <tui++/Component.h>
#include <tui++/Scrollable.h>

#include <functional>

namespace tui {

class Viewport: public Component {
  using base = Component;

public:
  Viewport();

  // The component displayed through this viewport (at most one).
  void set_view(const std::shared_ptr<Component> &view);
  std::shared_ptr<Component> get_view() const {
    return this->view;
  }

  // Position of the top-left visible corner, in view coordinates.
  void set_view_position(int x, int y);
  void set_view_position(Point const &position) {
    set_view_position(position.x, position.y);
  }

  Point get_view_position() const {
    return this->view_position;
  }

  // The full size of the view content (cells on the text screen). Decided by
  // the ScrollPane layout (which honors the view's Scrollable contract).
  void set_view_size(int width, int height);
  void set_view_size(Dimension const &size) {
    set_view_size(size.width, size.height);
  }

  Dimension get_view_size() const {
    return this->view_size;
  }

  // The visible area of the viewport itself.
  Dimension get_extent_size() const {
    return { get_width(), get_height() };
  }

  // Scrolls so that `rect` (view coordinates) is visible, keeping it as close
  // to the current position as possible.
  void scroll_rect_to_visible(Rectangle const &rect);

  // Fired after the view position changed.
  std::function<void()> on_view_position_changed;

  // Fired after the view content size changed (a re-layout is then needed).
  std::function<void()> on_view_size_changed;

  std::string to_string() const override {
    return describe("viewport");
  }

protected:
  virtual void paint(Graphics &g) override;

private:
  void clamp_view_position();
  void place_view();
  void notify_position_changed() {
    if (this->on_view_position_changed) {
      this->on_view_position_changed();
    }
  }

  std::shared_ptr<Component> view;
  Point view_position { };
  Dimension view_size { };
};

}
