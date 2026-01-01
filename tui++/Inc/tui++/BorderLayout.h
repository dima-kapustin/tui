#pragma once

#include <tui++/Layout.h>

#include <memory>
#include <optional>
#include <string_view>

namespace tui {

class Component;
class ComponentOrientation;

class BorderLayout: public AbstractLayout {
  int hgap;
  int vgap;

  std::shared_ptr<Component> north, west, east, south, center;

  /**
   * Relative positioning constants, that can be used instead of
   * north, south, east, west or center.
   * Mixing the two types of constants can lead to unpredictable results.  If
   * you use both types, the relative constants will take precedence.
   * For example, if you add components using both the {@code NORTH}
   * and {@code BEFORE_FIRST_LINE} constants in a container whose
   * orientation is {@code LEFT_TO_RIGHT}, only the
   * {@code BEFORE_FIRST_LINE} will be laid out.
   * This will be the same for last_line, first_item, last_item.
   */
  std::shared_ptr<Component> first_line, last_line, first_item, last_item;

public:
  /**
   * The north layout constraint (top of container).
   */
  constexpr static std::string_view NORTH = "North";

  /**
   * The south layout constraint (bottom of container).
   */
  constexpr static std::string_view SOUTH = "South";

  /**
   * The east layout constraint (right side of container).
   */
  constexpr static std::string_view EAST = "East";

  /**
   * The west layout constraint (left side of container).
   */
  constexpr static std::string_view WEST = "West";

  /**
   * The center layout constraint (middle of container).
   */
  constexpr static std::string_view CENTER = "Center";

  /**
   * The component comes before the first line of the layout's content.
   * For Western, left-to-right and top-to-bottom orientations, this is
   * equivalent to NORTH.
   */
  constexpr static std::string_view PAGE_START = "First";

  /**
   * The component comes after the last line of the layout's content.
   * For Western, left-to-right and top-to-bottom orientations, this is
   * equivalent to SOUTH.
   */
  constexpr static std::string_view PAGE_END = "Last";

  /**
   * The component goes at the beginning of the line direction for the
   * layout. For Western, left-to-right and top-to-bottom orientations,
   * this is equivalent to WEST.
   */
  constexpr static std::string_view LINE_START = "Before";

  /**
   * The component goes at the end of the line direction for the
   * layout. For Western, left-to-right and top-to-bottom orientations,
   * this is equivalent to EAST.
   */
  constexpr static std::string_view LINE_END = "After";

private:
  std::shared_ptr<Component> get_north_component() noexcept (true);
  std::shared_ptr<Component> get_west_component(const ComponentOrientation &orientation) noexcept (true);
  std::shared_ptr<Component> get_south_component() noexcept (true);
  std::shared_ptr<Component> get_east_component(const ComponentOrientation &orientation) noexcept (true);

  std::shared_ptr<Component> get_layout_component(const ComponentOrientation &orientation, const std::string_view &constraints) noexcept (false);

public:
  BorderLayout() :
      BorderLayout(0, 0) {
  }

  /**
   * Constructs a border layout with the specified gaps
   * between components.
   * The horizontal gap is specified by {@code hgap}
   * and the vertical gap is specified by {@code vgap}.
   */
  BorderLayout(int hgap, int vgap) :
      hgap(hgap), vgap(vgap) {
  }

public:
  void add_layout_component(const std::shared_ptr<Component> &c, const Constraints &constraints) noexcept (false) override;
  void remove_layout_component(const std::shared_ptr<Component> &c) override;

  /**
   * Gets the component that was added using the given constraint
   */
  std::shared_ptr<Component> get_layout_component(const Constraints &constraints) noexcept (false);
  /**
   * Returns the component that corresponds to the given constraint location
   * based on the component orientation.
   * Components added with the relative constraints {@code PAGE_START},
   * {@code PAGE_END}, {@code LINE_START}, and {@code LINE_END}
   * take precedence over components added with the explicit constraints
   * {@code NORTH}, {@code SOUTH}, {@code WEST}, and {@code EAST}.
   */
  std::shared_ptr<Component> get_layout_component(const ComponentOrientation &orientation, const Constraints &constraints) noexcept (false);

  /**
   * Gets the constraints for the specified component
   */
  std::optional<std::string_view> get_constraints(const std::shared_ptr<Component> &target);

  std::optional<Dimension> get_minimum_layout_size(const std::shared_ptr<const Component> &target) override;
  std::optional<Dimension> get_maximum_layout_size(const std::shared_ptr<const Component> &target) override;
  std::optional<Dimension> get_preferred_layout_size(const std::shared_ptr<const Component> &target) override;

  void layout(const std::shared_ptr<Component> &target) override;
};

}
