#include <tui++/DefaultFocusTraversalPolicy.h>

#include <tui++/Window.h>
#include <tui++/Component.h>

namespace tui {

bool DefaultFocusTraversalPolicy::accept(const std::shared_ptr<Component> &candidate) const {
  if (not (candidate->is_visible() and candidate->is_displayable() and candidate->is_enabled())) {
    return false;
  }

  // Verify that the Component is recursively enabled. Disabling a
  // heavyweight Container disables its children, whereas disabling
  // a lightweight Container does not.
  if (not is_a<Window>(candidate)) {
    for (auto parent = candidate->get_parent(); parent; parent = parent->get_parent()) {
      if (not parent->is_enabled()) {
        return false;
      }

      if (is_a<Window>(parent)) {
        break;
      }
    }
  }

  // TODO
//  auto focusable = candidate->is_focusable();
//  if (candidate->is_focus_traversable_overridden()) {
//    return focusable;
//  }

  return false;
}

}
