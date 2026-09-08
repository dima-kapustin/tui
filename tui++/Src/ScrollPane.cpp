#include <tui++/ScrollPane.h>
#include <tui++/Scrollable.h>
#include <tui++/Component.h>

#include <algorithm>

namespace tui {

namespace {

// A scroll bar is one terminal cell thick.
constexpr int VERTICAL_BAR_WIDTH = 1;
constexpr int HORIZONTAL_BAR_HEIGHT = 1;

// How many layout passes the bar-visibility decision may iterate. Each pass
// resolves the view content size for the current bar set and updates the
// flags; two or three passes settle any real change (Swing's
// ScrollPaneLayout::layoutContainer iterates the same way).
constexpr int MAX_LAYOUT_PASSES = 4;

}

ScrollPane::ScrollPane() {
  set_background_color(Color { 8, 8, 12 });
}

void ScrollPane::init() {
  base::init();
  // The children are created here (not in the constructor): their own init()
  // registers listeners, which needs shared ownership.
  this->viewport = make_component<Viewport>();
  this->vertical_bar = make_component<ScrollBar>(Orientation::VERTICAL);
  this->horizontal_bar = make_component<ScrollBar>(Orientation::HORIZONTAL);

  set_layout(std::make_shared<ScrollPaneLayout>(*this));

  add(this->viewport, std::string_view { "CENTER" });
  add(this->vertical_bar, std::string_view { "EAST" });
  add(this->horizontal_bar, std::string_view { "SOUTH" });

  // The viewport and the scroll bars share the scroll position: a bar moves
  // the viewport, a moved viewport (caret scrolls, wheel, drag) moves the
  // bars. Each direction is guarded by an equality check, so a change that
  // did not actually move anything cannot ping-pong.
  this->viewport->on_view_position_changed = [this] {
    sync_scroll_bar_values();
  };

  hook_vertical_bar();
  hook_horizontal_bar();

  // Swing's BasicScrollPaneUI installs its wheel handler on the pane itself,
  // so the wheel scrolls wherever it lands on the pane -- the viewport
  // background (the view may not cover it) included -- and, thanks to the
  // ancestor-first wheel dispatch, even over the scroll bars: the bars' own
  // wheel handlers then apply only to standalone bars, exactly as in Swing.
  add_listener([this](MouseWheelEvent &e) {
    this->process_wheel(e);
    e.consume();
  });

  // When the content size changes outside a layout pass (the view reports a
  // new preferred size after its lazy line index grew), the bar ranges must
  // follow immediately -- waiting for the next validate would let the user
  // scroll with a stale maximum.
  this->viewport->on_view_size_changed = [this] {
    update_bar_ranges();
  };
}

void ScrollPane::hook_vertical_bar() {
  auto model = this->vertical_bar->get_model();
  model->add_change_listener([this] {
    auto value = this->vertical_bar->get_value();
    auto position = this->viewport->get_view_position();
    if (position.y != value) {
      this->viewport->set_view_position(position.x, value);
    }
  });
}

void ScrollPane::hook_horizontal_bar() {
  auto model = this->horizontal_bar->get_model();
  model->add_change_listener([this] {
    auto value = this->horizontal_bar->get_value();
    auto position = this->viewport->get_view_position();
    if (position.x != value) {
      this->viewport->set_view_position(value, position.y);
    }
  });
}

void ScrollPane::process_wheel(MouseWheelEvent &e) {
  auto rotation = e.wheel_rotation;
  if (rotation == 0) {
    return;
  }

  // Shift+wheel scrolls horizontally (Swing); without a shift the wheel
  // scrolls vertically, falling back to horizontal when the vertical bar is
  // not visible.
  auto orientation = Orientation::VERTICAL;
  auto bar = this->vertical_bar;
  if (bool(e.modifiers & InputEvent::SHIFT_DOWN) or not bar->is_visible()) {
    orientation = Orientation::HORIZONTAL;
    bar = this->horizontal_bar;
  }
  if (not bar->is_visible()) {
    return; // nothing to scroll in that direction
  }

  auto position = this->viewport->get_view_position();
  auto visible = Rectangle {
    position.x, position.y,
    std::max(1, this->viewport->get_width()), std::max(1, this->viewport->get_height())
  };

  // How far one unit scrolls: the view's Scrollable contract when it has one
  // (a text view scrolls by lines), the bar's unit increment otherwise.
  auto unit = bar->get_unit_increment();
  if (auto view = this->viewport->get_view()) {
    if (auto scrollable = std::dynamic_pointer_cast<Scrollable>(view)) {
      unit = std::max(1, scrollable->get_scrollable_unit_increment(visible, orientation));
    }
  }

  // Swing's MouseWheelEvent.getUnitsToScroll() is 3 on the usual platforms:
  // one notch scrolls three units.
  constexpr auto UNITS_PER_NOTCH = 3;
  auto amount = unit;
  if (rotation == 1 or rotation == -1) {
    // Swing's limitScroll: a single notch never scrolls past the visible
    // extent (the bar's block increment).
    amount = std::min(amount, std::max(1, bar->get_block_increment()));
  }
  auto delta = UNITS_PER_NOTCH * amount * (rotation < 0 ? -1 : 1);

  // A lazy view extends its index (and content size) to the target position
  // before the position change is clamped against it.
  if (auto view = this->viewport->get_view()) {
    if (auto scrollable = std::dynamic_pointer_cast<Scrollable>(view)) {
      scrollable->scrollable_prepare_wheel_scroll(visible, orientation, delta);
    }
  }

  if (orientation == Orientation::VERTICAL) {
    auto target = position.y + delta;
    if (target != position.y) {
      this->viewport->set_view_position(position.x, target);
    }
  } else {
    auto target = position.x + delta;
    if (target != position.x) {
      this->viewport->set_view_position(target, position.y);
    }
  }
}

void ScrollPane::sync_scroll_bar_values() {
  auto position = this->viewport->get_view_position();
  this->vertical_bar->get_model()->set_value(position.y);
  this->horizontal_bar->get_model()->set_value(position.x);
}

void ScrollPane::update_bar_ranges() {
  auto content = this->viewport->get_view_size();
  auto extent = this->viewport->get_extent_size();
  this->vertical_bar->get_model()->set_range_properties(this->viewport->get_view_position().y, extent.height, 0, std::max(0, content.height));
  this->horizontal_bar->get_model()->set_range_properties(this->viewport->get_view_position().x, extent.width, 0, std::max(0, content.width));
}

Dimension ScrollPane::resolve_view_size(int available_width, int available_height) const {
  auto view = this->viewport->get_view();
  if (not view) {
    return { 0, 0 };
  }
  auto preferred = view->get_preferred_size();
  if (auto scrollable = std::dynamic_pointer_cast<Scrollable>(view)) {
    // A Scrollable view decides for itself which dimension tracks the
    // viewport (a text component tracks the width and lets the height follow
    // its content) and which one is its content size.
    auto width = scrollable->get_scrollable_tracks_viewport_width() ? available_width : std::max(1, preferred.width);
    auto height = scrollable->get_scrollable_tracks_viewport_height() ? available_height : std::max(1, preferred.height);
    return { width, height };
  }
  return { preferred.width > 0 ? preferred.width : available_width, preferred.height > 0 ? preferred.height : available_height };
}

void ScrollPane::layout_pane() {
  auto insets = get_insets();
  auto width = get_width() - insets.left - insets.right;
  auto height = get_height() - insets.top - insets.bottom;
  if (width <= 0 or height <= 0) {
    return;
  }

  // Decide which bars are shown. A bar's presence shrinks the viewport, which
  // can change whether the other bar (or itself) is still needed, so iterate
  // until the flags settle.
  for (auto pass = 0; pass < MAX_LAYOUT_PASSES; ++pass) {
    auto vp_width = width - (this->vertical_visible ? VERTICAL_BAR_WIDTH : 0);
    auto vp_height = height - (this->horizontal_visible ? HORIZONTAL_BAR_HEIGHT : 0);
    if (vp_width < 1 or vp_height < 1) {
      // The pane is too small for both bars; keep the content at least one
      // cell by dropping the horizontal bar first (the vertical bar is the
      // one that scrolls text).
      this->horizontal_visible = false;
      vp_width = width - (this->vertical_visible ? VERTICAL_BAR_WIDTH : 0);
      vp_height = height;
    }
    auto content = resolve_view_size(std::max(1, vp_width), std::max(1, vp_height));
    auto want_vertical = this->vertical_policy == VERTICAL_SCROLLBAR_ALWAYS
        or (this->vertical_policy == VERTICAL_SCROLLBAR_AS_NEEDED and content.height > vp_height);
    auto want_horizontal = this->horizontal_policy == HORIZONTAL_SCROLLBAR_ALWAYS
        or (this->horizontal_policy == HORIZONTAL_SCROLLBAR_AS_NEEDED and content.width > vp_width);
    if (want_vertical == this->vertical_visible and want_horizontal == this->horizontal_visible) {
      break;
    }
    this->vertical_visible = want_vertical;
    this->horizontal_visible = want_horizontal;
  }

  auto vp_width = width - (this->vertical_visible ? VERTICAL_BAR_WIDTH : 0);
  auto vp_height = height - (this->horizontal_visible ? HORIZONTAL_BAR_HEIGHT : 0);
  auto content = resolve_view_size(std::max(1, vp_width), std::max(1, vp_height));

  // Swing's ScrollPaneLayout places the viewport in the center, the vertical
  // bar on the east edge and the horizontal bar on the south edge. The
  // vertical bar spans the pane's full height, so the corner cell belongs to
  // its column (it is painted after the horizontal bar).
  this->viewport->set_bounds(insets.left, insets.top, std::max(0, vp_width), std::max(0, vp_height));
  this->vertical_bar->set_visible(this->vertical_visible);
  this->horizontal_bar->set_visible(this->horizontal_visible);
  if (this->vertical_visible) {
    this->vertical_bar->set_bounds(insets.left + vp_width, insets.top, VERTICAL_BAR_WIDTH, height);
  }
  if (this->horizontal_visible) {
    // The horizontal bar spans the pane's width minus the vertical bar's
    // column: the corner cell belongs to the vertical bar (it is painted
    // after the horizontal bar), so the horizontal bar must not draw its
    // right arrow into it.
    this->horizontal_bar->set_bounds(insets.left, insets.top + vp_height, vp_width, HORIZONTAL_BAR_HEIGHT);
  }

  // Re-resolve the content size now that the bar set is final, then size the
  // view and update the bar ranges to the viewport's extent.
  this->viewport->set_view_size(content.width, content.height);
  update_bar_ranges();

  // Swing sizes the page step of a scroll bar to the visible extent.
  this->vertical_bar->set_block_increment(std::max(1, vp_height - 1));
  this->horizontal_bar->set_block_increment(std::max(1, vp_width - 1));
}

void ScrollPaneLayout::layout(const std::shared_ptr<Component> &target) {
  this->pane.layout_pane();
}

void ScrollPaneLayout::add_layout_component(const std::shared_ptr<Component> &component, const Constraints &constraints) {
  // The children are fixed (viewport, bars); constraints are accepted for
  // API compatibility but do not change the arrangement.
  (void)component;
  (void)constraints;
}

void ScrollPaneLayout::remove_layout_component(const std::shared_ptr<Component> &component) {
  // The pane owns its children; removing one from outside would break the
  // pane, so the layout simply forgets nothing. Swing forbids removing the
  // viewport/scrollbars of a JScrollPane for the same reason.
  (void)component;
}

std::optional<Dimension> ScrollPaneLayout::get_preferred_layout_size(const std::shared_ptr<const Component> &target) {
  auto insets = target->get_insets();
  auto width = insets.left + insets.right;
  auto height = insets.top + insets.bottom;

  auto view = this->pane.viewport->get_view();
  if (view) {
    auto view_size = Dimension { };
    if (auto scrollable = std::dynamic_pointer_cast<Scrollable>(view)) {
      view_size = scrollable->get_preferred_scrollable_viewport_size();
    } else {
      view_size = view->get_preferred_size();
    }
    width += view_size.width;
    height += view_size.height;
  } else {
    // A viewport without a view still takes a useful default size.
    width += 80;
    height += 24;
  }

  if (this->pane.vertical_policy == ScrollPane::VERTICAL_SCROLLBAR_ALWAYS) {
    width += VERTICAL_BAR_WIDTH;
  }
  if (this->pane.horizontal_policy == ScrollPane::HORIZONTAL_SCROLLBAR_ALWAYS) {
    height += HORIZONTAL_BAR_HEIGHT;
  }
  return Dimension { width, height };
}

}
