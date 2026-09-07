// Unit tests for the JScrollPane-style machinery: bar visibility policies,
// layout geometry, and the scroll bar model <-> viewport position sync.

#include <tui++/BoundedRangeModel.h>
#include <tui++/ScrollPane.h>
#include <tui++/Scrollable.h>
#include <tui++/Viewport.h>
#include <tui++/Component.h>
#include <tui++/Dimension.h>

#include <cassert>
#include <cstdio>
#include <memory>

using namespace tui;

namespace {

// A view whose size is whatever the pane's layout resolves it to (both
// dimensions follow the viewport). Useful when only one bar should appear.
class TrackingView: public Component, public Scrollable {
public:
  Dimension get_preferred_scrollable_viewport_size() const override {
    return { 30, 10 };
  }
  int get_scrollable_unit_increment(Rectangle const &, Orientation) const override {
    return 1;
  }
  int get_scrollable_block_increment(Rectangle const &visible, Orientation orientation) const override {
    return orientation == Orientation::VERTICAL ? std::max(1, visible.height - 1) : std::max(1, visible.width - 1);
  }
  bool get_scrollable_tracks_viewport_width() const override {
    return true;
  }
  bool get_scrollable_tracks_viewport_height() const override {
    return true;
  }
};

// A view with fixed content size, bigger than the pane on both axes.
class BigFixedView: public Component, public Scrollable {
public:
  BigFixedView() {
    set_preferred_size(Dimension { 100, 500 });
  }
  Dimension get_preferred_scrollable_viewport_size() const override {
    return { 40, 10 };
  }
  int get_scrollable_unit_increment(Rectangle const &, Orientation) const override {
    return 1;
  }
  int get_scrollable_block_increment(Rectangle const &visible, Orientation orientation) const override {
    return orientation == Orientation::VERTICAL ? std::max(1, visible.height - 1) : std::max(1, visible.width - 1);
  }
  bool get_scrollable_tracks_viewport_width() const override {
    return false;
  }
  bool get_scrollable_tracks_viewport_height() const override {
    return false;
  }
};

void layout_pane(std::shared_ptr<ScrollPane> const &pane, int width, int height) {
  pane->set_bounds(0, 0, width, height);
  pane->validate();
}

// A ScrollPane wires its children up in init(), so it is created with
// make_component (like Window/RootPane in this framework).
std::shared_ptr<ScrollPane> make_pane() {
  return make_component<ScrollPane>();
}

void test_bar_geometry() {
  auto pane = make_pane();
  auto view = std::make_shared<BigFixedView>();
  pane->set_viewport_view(view);
  layout_pane(pane, 40, 10);

  // Content 100x500 in a 40x10 pane: both bars needed. The pane gives the
  // vertical bar its own column (south bar spans the full width, the corner
  // belongs to the vertical bar's column).
  auto viewport = pane->get_viewport();
  assert(viewport->get_bounds().width == 39);
  assert(viewport->get_bounds().height == 9);
  auto vertical = pane->get_vertical_scroll_bar();
  auto horizontal = pane->get_horizontal_scroll_bar();
  assert(vertical->is_visible());
  assert(horizontal->is_visible());
  assert(vertical->get_bounds().x == 39 and vertical->get_bounds().width == 1 and vertical->get_bounds().height == 10);
  assert(horizontal->get_bounds().y == 9 and horizontal->get_bounds().height == 1);

  // The bar models mirror the viewport extent and the content size.
  assert(vertical->get_model()->get_extent() == 9);
  assert(vertical->get_model()->get_maximum() == 500);
  assert(horizontal->get_model()->get_extent() == 39);
  assert(horizontal->get_model()->get_maximum() == 100);

  // The view content size is the resolved size.
  assert(viewport->get_view_size().width == 100);
  assert(viewport->get_view_size().height == 500);
}

void test_vertical_only() {
  // A narrow, tall view in a wide pane: only the vertical bar shows.
  auto pane = make_pane();
  auto view = std::make_shared<BigFixedView>();
  pane->set_viewport_view(view);
  layout_pane(pane, 120, 10);

  auto vertical = pane->get_vertical_scroll_bar();
  auto horizontal = pane->get_horizontal_scroll_bar();
  assert(vertical->is_visible());
  assert(not horizontal->is_visible());
  assert(pane->get_viewport()->get_bounds().width == 119); // full width, no hbar
  assert(pane->get_viewport()->get_bounds().height == 10); // no horizontal bar row
}

void test_no_bars_when_content_fits() {
  auto pane = make_pane();
  auto view = std::make_shared<BigFixedView>();
  pane->set_viewport_view(view);
  layout_pane(pane, 120, 600);

  assert(not pane->get_vertical_scroll_bar()->is_visible());
  assert(not pane->get_horizontal_scroll_bar()->is_visible());
}

void test_tracking_view() {
  // A view that tracks both axes never needs a bar (content == viewport).
  auto pane = make_pane();
  auto view = std::make_shared<TrackingView>();
  pane->set_viewport_view(view);
  layout_pane(pane, 50, 20);

  assert(not pane->get_vertical_scroll_bar()->is_visible());
  assert(not pane->get_horizontal_scroll_bar()->is_visible());
  assert(pane->get_viewport()->get_view_size().width == 50);
  assert(pane->get_viewport()->get_view_size().height == 20);
}

void test_policy_always() {
  auto pane = make_pane();
  auto view = std::make_shared<TrackingView>();
  pane->set_viewport_view(view);
  pane->set_vertical_scroll_bar_policy(ScrollPane::VERTICAL_SCROLLBAR_ALWAYS);
  pane->set_horizontal_scroll_bar_policy(ScrollPane::HORIZONTAL_SCROLLBAR_ALWAYS);
  layout_pane(pane, 50, 20);

  assert(pane->get_vertical_scroll_bar()->is_visible());
  assert(pane->get_horizontal_scroll_bar()->is_visible());
  assert(pane->get_viewport()->get_bounds().width == 49);
  assert(pane->get_viewport()->get_bounds().height == 19);

  pane->set_vertical_scroll_bar_policy(ScrollPane::VERTICAL_SCROLLBAR_NEVER);
  pane->invalidate(); // revalidate() needs a parent; a bare pane must ask itself
  pane->validate();
  assert(not pane->get_vertical_scroll_bar()->is_visible());
  assert(pane->get_viewport()->get_bounds().width == 50);
}

void test_scroll_sync() {
  auto pane = make_pane();
  auto view = std::make_shared<BigFixedView>();
  pane->set_viewport_view(view);
  layout_pane(pane, 40, 10);

  auto viewport = pane->get_viewport();
  auto vertical = pane->get_vertical_scroll_bar();
  auto horizontal = pane->get_horizontal_scroll_bar();

  // Moving the viewport moves the bar models...
  viewport->set_view_position(10, 20);
  assert(vertical->get_value() == 20);
  assert(horizontal->get_value() == 10);

  // ... and moving the bar models moves the viewport (no ping-pong).
  vertical->set_value(50);
  assert(viewport->get_view_position().y == 50);
  assert(vertical->get_value() == 50);
  horizontal->set_value(30);
  assert(viewport->get_view_position().x == 30);
  assert(horizontal->get_value() == 30);

  // Values clamp to the content extent: maximum - extent.
  vertical->set_value(10000);
  assert(viewport->get_view_position().y == 500 - 9);
  assert(vertical->get_value() == 500 - 9);
}

void test_view_size_growth() {
  // The classic text-viewer case: the content height grows while the pane is
  // shown (a lazily indexed file). The bar range must follow the view size.
  auto pane = make_pane();
  auto view = std::make_shared<BigFixedView>();
  pane->set_viewport_view(view);
  layout_pane(pane, 40, 10);
  auto vertical = pane->get_vertical_scroll_bar();
  assert(vertical->get_model()->get_maximum() == 500);

  view->set_preferred_size(Dimension { 100, 700 });
  pane->get_viewport()->set_view_size(100, 700);
  assert(vertical->get_model()->get_maximum() == 700);
  assert(vertical->get_model()->get_maximum_value() == 700 - 9);
}

void test_model_invariants() {
  // Swing's DefaultBoundedRangeModel invariants: value in
  // [minimum, maximum - extent]; adjusting one property adjusts the others.
  DefaultBoundedRangeModel model;
  model.set_range_properties(50, 20, 0, 100);
  assert(model.get_value() == 50 and model.get_extent() == 20);
  assert(model.get_maximum_value() == 80);

  model.set_value(10000);
  assert(model.get_value() == 80); // clamped to maximum - extent

  model.set_maximum(30);
  assert(model.get_value() == 10); // clamped again: 30 - 20

  model.set_extent(0);
  assert(model.get_value() == 10); // the extent is gone, the value stays
}

} // namespace

void test_ScrollPane() {
  test_bar_geometry();
  test_vertical_only();
  test_no_bars_when_content_fits();
  test_tracking_view();
  test_policy_always();
  test_scroll_sync();
  test_view_size_growth();
  test_model_invariants();
  std::fprintf(stderr, "test_ScrollPane: ok\n");
}
