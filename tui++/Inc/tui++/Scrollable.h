#pragma once

// Swing's Scrollable interface: implemented by components whose scrolling
// behavior needs to be smarter than "scroll by preferred size" -- e.g. a text
// component that scrolls by lines and tracks the viewport width.

#include <tui++/Dimension.h>
#include <tui++/Orientation.h>
#include <tui++/Rectangle.h>

namespace tui {

class Scrollable {
public:
  virtual ~Scrollable() = default;

  // The size of the viewport the component would like to be shown in (for a
  // text component, a comfortable number of lines).
  virtual Dimension get_preferred_scrollable_viewport_size() const = 0;

  // How far a unit (one wheel notch / one arrow click) scrolls, given the
  // currently visible rectangle of the viewport. The visible rectangle is in
  // the component's own coordinate space.
  virtual int get_scrollable_unit_increment(Rectangle const &visible_rect, Orientation orientation) const = 0;

  // How far a block (page) scrolls, given the currently visible rectangle.
  virtual int get_scrollable_block_increment(Rectangle const &visible_rect, Orientation orientation) const = 0;

  // Whether the component's width/height should be forced to the viewport's
  // (no horizontal/vertical scrolling in that direction).
  virtual bool get_scrollable_tracks_viewport_width() const = 0;
  virtual bool get_scrollable_tracks_viewport_height() const = 0;

  // Called by the scroll pane's wheel handler before it applies a wheel
  // scroll, so a view with a lazily indexed content (TextArea) can extend
  // its index and content size to cover the target position before the
  // viewport clamps the move. The visible rectangle is in the view's own
  // coordinates and `amount` is the signed scroll distance in cells. The
  // default does nothing.
  virtual void scrollable_prepare_wheel_scroll(Rectangle const &visible_rect, Orientation orientation, int amount) {
    (void)visible_rect;
    (void)orientation;
    (void)amount;
  }
};

}
