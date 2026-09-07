#pragma once

// ScrollPane - Swing's JScrollPane: a viewport with optional scroll bars.
//
//   ScrollPane pane;
//   pane.set_viewport_view(some_component);
//   pane.set_vertical_scroll_bar_policy(ScrollPane::VERTICAL_SCROLLBAR_AS_NEEDED);
//
// The layout resolves the view's size from its Scrollable contract when it
// implements one (tracks_viewport_width/height, preferred size); otherwise it
// falls back to the view's preferred size. Bars appear per the policies; a
// corner cell fills the gap when both bars are shown.

#include <tui++/Component.h>
#include <tui++/Layout.h>
#include <tui++/ScrollBar.h>
#include <tui++/Viewport.h>

namespace tui {

class ScrollPane: public Component {
  using base = Component;

public:
  enum ScrollBarPolicy {
    VERTICAL_SCROLLBAR_AS_NEEDED = 0,
    VERTICAL_SCROLLBAR_ALWAYS = 1,
    VERTICAL_SCROLLBAR_NEVER = 2,
    HORIZONTAL_SCROLLBAR_AS_NEEDED = 0,
    HORIZONTAL_SCROLLBAR_ALWAYS = 1,
    HORIZONTAL_SCROLLBAR_NEVER = 2,
  };

public:
  ScrollPane();

  std::shared_ptr<Viewport> get_viewport() const {
    return this->viewport;
  }

  std::shared_ptr<ScrollBar> get_vertical_scroll_bar() const {
    return this->vertical_bar;
  }

  std::shared_ptr<ScrollBar> get_horizontal_scroll_bar() const {
    return this->horizontal_bar;
  }

  // The component shown inside the viewport.
  void set_viewport_view(const std::shared_ptr<Component> &view) {
    this->viewport->set_view(view);
    revalidate();
  }

  void set_vertical_scroll_bar_policy(int policy) {
    this->vertical_policy = policy;
    revalidate();
  }

  int get_vertical_scroll_bar_policy() const {
    return this->vertical_policy;
  }

  void set_horizontal_scroll_bar_policy(int policy) {
    this->horizontal_policy = policy;
    revalidate();
  }

  int get_horizontal_scroll_bar_policy() const {
    return this->horizontal_policy;
  }

  // Whether the scroll bars are currently visible (as decided by the last
  // layout pass according to the policies).
  bool vertical_bar_needed() const {
    return this->vertical_visible;
  }
  bool horizontal_bar_needed() const {
    return this->horizontal_visible;
  }

  std::string to_string() const override {
    return describe("scroll pane");
  }

  friend class ScrollPaneLayout;

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  // Called by make_component once the pane is owned by a shared pointer:
  // adding the viewport and the bars here (instead of in the constructor)
  // lets add() use shared_from_this safely.
  void init() override;

private:
  // The layout entry point (invoked by ScrollPaneLayout::layout).
  void layout_pane();

  // Content size of the view given the available viewport area and the view's
  // Scrollable contract.
  Dimension resolve_view_size(int available_width, int available_height) const;

  void sync_scroll_bars();
  void update_bar_ranges();

  void hook_vertical_bar();
  void hook_horizontal_bar();

  std::shared_ptr<Viewport> viewport;
  std::shared_ptr<ScrollBar> vertical_bar;
  std::shared_ptr<ScrollBar> horizontal_bar;

  int vertical_policy = VERTICAL_SCROLLBAR_AS_NEEDED;
  int horizontal_policy = HORIZONTAL_SCROLLBAR_AS_NEEDED;

  // Visibility as decided by the most recent layout pass.
  bool vertical_visible = true;
  bool horizontal_visible = false;

  // Wire the scroll bar models to the viewport position (value <- position)
  // and the viewport position to the models (value -> position). Every path
  // is guarded by an equality check, so no recursion flag is needed.
  void sync_scroll_bar_values();
};

// The pane's layout manager: viewport in the center, scroll bars on the
// south/east edges (insets-aware), a corner cell when both bars are shown.
class ScrollPaneLayout final: public Layout {
public:
  explicit ScrollPaneLayout(ScrollPane &pane) :
      pane(pane) {
  }

  std::optional<Dimension> get_preferred_layout_size(const std::shared_ptr<const Component> &target) override;
  std::optional<Dimension> get_minimum_layout_size(const std::shared_ptr<const Component> &target) override {
    return get_preferred_layout_size(target);
  }
  std::optional<Dimension> get_maximum_layout_size(const std::shared_ptr<const Component> &target) override {
    return { Dimension::max() };
  }
  void layout(const std::shared_ptr<Component> &target) override;
  void add_layout_component(const std::shared_ptr<Component> &component, const Constraints &constraints) override;
  void remove_layout_component(const std::shared_ptr<Component> &component) override;
  float get_layout_alignment_x(const std::shared_ptr<const Component> &) const override {
    return 0;
  }
  float get_layout_alignment_y(const std::shared_ptr<const Component> &) const override {
    return 0;
  }
  void invalidate_layout(const std::shared_ptr<const Component> &) override {
  }

private:
  ScrollPane &pane;
};

}
