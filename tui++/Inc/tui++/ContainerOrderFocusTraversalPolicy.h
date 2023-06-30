#pragma once

#include <tui++/FocusTraversalPolicy.h>

#include <memory>
#include <vector>
#include <algorithm>

namespace tui {

class Component;

class ContainerOrderFocusTraversalPolicy: public FocusTraversalPolicy {
  using FocusTraversalCycle = std::vector<std::shared_ptr<Component>>;

  enum FocusTraversalOrder {
    /**
     * This constant is used when the forward focus traversal order is active.
     */
    FORWARD_TRAVERSAL,

    /**
     * This constant is used when the backward focus traversal order is active.
     */
    BACKWARD_TRAVERSAL
  };

  /**
   * Whether this {@code ContainerOrderFocusTraversalPolicy} transfers focus
   * down-cycle implicitly.
   */
  bool implicitDownCycleTraversal = true;

  /**
   * Used by getComponentAfter and getComponentBefore for efficiency. In
   * order to maintain compliance with the specification of
   * FocusTraversalPolicy, if traversal wraps, we should invoke
   * getFirstComponent or getLastComponent. These methods may be overriden in
   * subclasses to behave in a non-generic way. However, in the generic case,
   * these methods will simply return the first or last Components of the
   * sorted list, respectively. Since getComponentAfter and
   * getComponentBefore have already built the list before determining
   * that they need to invoke getFirstComponent or getLastComponent, the
   * list should be reused if possible.
   */
  mutable std::shared_ptr<Component> cachedRoot;
  mutable FocusTraversalCycle cachedCycle;

private:
  /*
   * We suppose to use getFocusTraversalCycle & getComponentIndex methods in order
   * to divide the policy into two parts:
   * 1) Making the focus traversal cycle.
   * 2) Traversing the cycle.
   * The 1st point assumes producing a list of components representing the focus
   * traversal cycle. The two methods mentioned above should implement this logic.
   * The 2nd point assumes implementing the common concepts of operating on the
   * cycle: traversing back and forth, retrieving the initial/default/first/last
   * component. These concepts are described in the AWT Focus Spec and they are
   * applied to the FocusTraversalPolicy in general.
   * Thus, a descendant of this policy may wish to not reimplement the logic of
   * the 2nd point but just override the implementation of the 1st one.
   * A striking example of such a descendant is the javax.swing.SortingFocusTraversalPolicy.
   */
  FocusTraversalCycle get_focus_traversal_cycle(const std::shared_ptr<Component> &aContainer) const {
    auto cycle = FocusTraversalCycle { };
    enumerate_cycle(aContainer, cycle);
    return cycle;
  }

  size_t get_component_index(const std::vector<std::shared_ptr<Component>> &cycle, const std::shared_ptr<Component> &aComponent) const {
    auto pos = std::find(cycle.begin(), cycle.end(), aComponent);
    return pos == cycle.end() ? -1U : (pos - cycle.begin());
  }

  void enumerate_cycle(const std::shared_ptr<Component> &container, FocusTraversalCycle &cycle) const;

  std::shared_ptr<Component> get_topmost_provider(const std::shared_ptr<Component> &focus_cycle_root, const std::shared_ptr<Component> &aComponent) const;

  /*
   * Checks if a new focus cycle takes place and returns a Component to traverse focus to.
   * @param comp a possible focus cycle root or policy provider
   * @param traversalDirection the direction of the traversal
   * @return a Component to traverse focus to if {@code comp} is a root or provider
   *         and implicit down-cycle is set, otherwise {@code null}
   */
  std::shared_ptr<Component> get_component_down_cycle(const std::shared_ptr<Component> &comp, FocusTraversalOrder traversal_direction) const;

protected:
  /**
   * Determines whether a Component is an acceptable choice as the new
   * focus owner. By default, this method will accept a Component if and
   * only if it is visible, displayable, enabled, and focusable.
   */
  virtual bool accept(const std::shared_ptr<Component> &aComponent) const;

public:
  std::shared_ptr<Component> get_component_after(const std::shared_ptr<Component> &container, const std::shared_ptr<Component> &component) const override;

  std::shared_ptr<Component> get_component_before(const std::shared_ptr<Component> &container, const std::shared_ptr<Component> &component) const override;

  std::shared_ptr<Component> get_first_component(const std::shared_ptr<Component> &container) const override;

  std::shared_ptr<Component> get_last_component(const std::shared_ptr<Component> &container) const override;

  std::shared_ptr<Component> get_default_component(const std::shared_ptr<Component> &container) const override;
};

}
