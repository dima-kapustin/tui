#pragma once

#include <tui++/Layout.h>

namespace tui {

class FlowLayout: public AbstractLayout {
public:
  enum Alignment {
    /**
     * This value indicates that each row of components should be left-justified.
     */
    LEFT = 0,

    /**
     * This value indicates that each row of components should be centered.
     */
    CENTER = 1,

    /**
     * This value indicates that each row of components should be right-justified.
     */
    RIGHT = 2,

    /**
     * This value indicates that each row of components should be justified to the leading edge of the container's orientation, for example,
     * to the left in left-to-right orientations.
     */
    LEADING = 3,

    /**
     * This value indicates that each row of components should be justified to the trailing edge of the container's orientation, for
     * example, to the right in left-to-right orientations.
     */
    TRAILING = 4,
  };

private:
  Alignment align;

  /**
   * The flow layout manager allows a seperation of components with gaps. The horizontal gap will specify the space between components and
   * between the components and the borders of the <code>Container</code>.
   */
  int hgap;

  /**
   * The flow layout manager allows a seperation of components with gaps. The vertical gap will specify the space between rows and between
   * the the rows and the borders of the <code>Container</code>.
   */
  int vgap;

private:
  int move_components(Component &target, int x, int y, int width, int height, size_t row_start, size_t row_end, bool ltr);

public:
  /**
   * Constructs a new <code>FlowLayout</code> with a centered alignment and a default 5-unit horizontal and vertical gap.
   */
  FlowLayout() :
      FlowLayout(CENTER, 1, 0) {
  }

  /**
   * Creates a new flow layout manager with the indicated alignment and the indicated horizontal and vertical gaps.
   */
  FlowLayout(Alignment align, int hgap, int vgap) :
      align(align), hgap(hgap), vgap(vgap) {
  }

public:
  std::optional<Dimension> get_minimum_layout_size(const std::shared_ptr<const Component> &target) override;

  std::optional<Dimension> get_preferred_layout_size(const std::shared_ptr<const Component> &target) override;

  void layout(const std::shared_ptr<Component> &target) override;
};

}
