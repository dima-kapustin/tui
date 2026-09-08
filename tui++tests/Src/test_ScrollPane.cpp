// Unit tests for the JScrollPane-style machinery: bar visibility policies,
// layout geometry, and the scroll bar model <-> viewport position sync.

#include <tui++/BoundedRangeModel.h>
#include <tui++/ScrollPane.h>
#include <tui++/Scrollable.h>
#include <tui++/Viewport.h>
#include <tui++/Component.h>
#include <tui++/Dimension.h>
#include <tui++/Frame.h>
#include <tui++/Screen.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>

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

// Drains the event queue (repaint invocations posted by set_visible etc.).
static void drain() {
  while (screen.get_event_queue().pop(std::chrono::milliseconds::zero())) {
  }
}

void test_scrollbar_mouse_drag() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 40, 10 });
  auto pane = make_component<ScrollPane>();
  frame->add(pane);
  auto view = std::make_shared<BigFixedView>();
  pane->set_viewport_view(view);
  frame->set_visible(true);
  drain();

  auto vertical = pane->get_vertical_scroll_bar();
  auto viewport = pane->get_viewport();
  assert(vertical->is_visible());

  // The thumb in frame-local coordinates.
  auto bar_loc = vertical->get_location_on_screen();
  auto thumb = vertical->get_thumb_rect();
  assert(thumb.width > 0 and thumb.height > 0);
  auto thumb_center = convert_point_from_screen(
      Point { bar_loc.x + thumb.x + thumb.width / 2, bar_loc.y + thumb.y + thumb.height / 2 }, frame);

  auto initial = viewport->get_view_position().y;

  // Press the thumb (the terminal reports a press with the button already
  // down, so was_button_down_before() sees it as just-released).
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_PRESSED, MousePressEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, thumb_center.x, thumb_center.y, false);
  frame->dispatch_event(*screen.get_event_queue().pop());
  drain();

  // Drag the thumb down by a few rows, then release.
  auto drag = Point { thumb_center.x, thumb_center.y + 3 };
  screen.post<MouseDragEvent>(frame, MouseEvent::LEFT_BUTTON, InputEvent::LEFT_BUTTON_DOWN, drag.x, drag.y);
  frame->dispatch_event(*screen.get_event_queue().pop());
  drain();
  screen.post<MousePressEvent>(frame, MousePressEvent::MOUSE_RELEASED, MousePressEvent::LEFT_BUTTON, InputEvent::NO_MODIFIERS, drag.x, drag.y, false);
  frame->dispatch_event(*screen.get_event_queue().pop());
  drain();

  assert(viewport->get_view_position().y > initial && "dragging the thumb must scroll the viewport");

  frame->set_visible(false);
  drain();
}

// A TextArea with line_wrap off (the Swing default) has a natural width: the
// longest line. When that exceeds the viewport, the pane shows a horizontal
// scroll bar; turning line wrapping on forces the width back to the viewport.
void test_text_area_horizontal_scroll() {
  auto pane = make_pane();
  auto area = make_component<TextArea>();
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, "short\n" + std::string(120, 'x') + "\ntail\n");
  buffer->scan_to_end();
  area->set_buffer(buffer);
  pane->set_viewport_view(area);
  layout_pane(pane, 40, 10);

  assert(not area->is_line_wrap());
  assert(pane->horizontal_bar_needed());
  assert(pane->get_horizontal_scroll_bar()->is_visible());
  // The content width is the longest line, not the viewport width.
  assert(pane->get_viewport()->get_view_size().width >= 120);

  // Enabling wrap tracks the viewport width: no horizontal overflow.
  area->set_line_wrap(true);
  pane->validate();
  assert(not pane->horizontal_bar_needed());
  assert(not pane->get_horizontal_scroll_bar()->is_visible());

  area->set_line_wrap(false);
  pane->validate();
  assert(pane->horizontal_bar_needed());
}

} // namespace

// The wheel scrolls wherever it lands on the pane (Swing's BasicScrollPaneUI
// installs its wheel handler on the pane): over the view's own background
// (when the content does not fill the viewport), over the view (the event
// bubbles from the view to the pane), and even over the scroll bars (the
// pane's handler shadows the bars' own, as in Swing). One notch scrolls by
// three units (Swing's MouseWheelEvent.getUnitsToScroll()), Shift+wheel
// scrolls horizontally, and the wheel falls back to the horizontal bar when
// the vertical one is hidden.
void test_pane_wheel_scroll() {
  terminal.set_type("text");

  auto frame = make_component<Frame>();
  frame->set_size({ 40, 10 });
  auto pane = make_component<ScrollPane>();
  frame->add(pane);
  auto area = make_component<TextArea>();
  auto buffer = TextBuffer::create_empty();
  std::string text;
  for (auto i = 0; i < 100; ++i) {
    text += "short line\n";
  }
  buffer->replace(0, 0, text);
  buffer->scan_to_end();
  area->set_buffer(buffer);
  pane->set_viewport_view(area);
  frame->set_visible(true);
  drain();

  auto viewport = pane->get_viewport();
  auto vertical = pane->get_vertical_scroll_bar();
  assert(vertical->is_visible());
  assert(viewport->get_view_position().y == 0);
  // The content is narrow: the area right of the text is viewport background.
  assert(area->get_width() < viewport->get_width());

  // Wheel over the background (the black area right of the text): three
  // units per notch.
  viewport->dispatch_event<MouseWheelEvent>(InputEvent::NO_MODIFIERS, 30, 5, 1);
  drain();
  assert(viewport->get_view_position().y == 3);

  // Wheel over the text bubbles from the view to the pane.
  area->dispatch_event<MouseWheelEvent>(InputEvent::NO_MODIFIERS, 3, 5, 1);
  drain();
  assert(viewport->get_view_position().y == 6);

  // Wheel over the scroll bar scrolls by the pane's unit handling too (the
  // bar's own block-increment handler applies only to standalone bars).
  vertical->dispatch_event<MouseWheelEvent>(InputEvent::NO_MODIFIERS, 0, 2, 1);
  drain();
  assert(viewport->get_view_position().y == 9);

  // Shift+wheel would scroll horizontally, but the narrow content needs no
  // horizontal bar: nothing moves.
  area->dispatch_event<MouseWheelEvent>(InputEvent::SHIFT_DOWN, 3, 5, 1);
  drain();
  assert(viewport->get_view_position().y == 9);
  assert(viewport->get_view_position().x == 0);

  // A wide document: the horizontal bar appears, and Shift+wheel scrolls
  // sideways by three units per notch.
  auto wide = TextBuffer::create_empty();
  wide->replace(0, 0, std::string(120, 'x') + "\n" + std::string(120, 'y') + "\n" + std::string(120, 'z') + "\n");
  wide->scan_to_end();
  area->set_buffer(wide);
  frame->validate();
  drain();
  assert(pane->get_horizontal_scroll_bar()->is_visible());
  assert(viewport->get_view_position().x == 0);
  area->dispatch_event<MouseWheelEvent>(InputEvent::SHIFT_DOWN, 3, 3, 1);
  drain();
  assert(viewport->get_view_position().x == 3);

  frame->set_visible(false);
  drain();
}

void test_ScrollPane() {
  test_bar_geometry();
  test_vertical_only();
  test_no_bars_when_content_fits();
  test_tracking_view();
  test_policy_always();
  test_scroll_sync();
  test_view_size_growth();
  test_model_invariants();
  test_scrollbar_mouse_drag();
  test_text_area_horizontal_scroll();
  test_pane_wheel_scroll();
  std::fprintf(stderr, "test_ScrollPane: ok\n");
}
