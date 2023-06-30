#include <tui++/DefaultFocusTraversalPolicy.h>

#include <tui++/Window.h>
#include <tui++/Component.h>

namespace tui {

bool DefaultFocusTraversalPolicy::accept(const std::shared_ptr<Component> &aComponent) const {
  if (not (aComponent->is_visible() and aComponent->is_displayable() and aComponent->is_enabled())) {
    return false;
  }

  // Verify that the Component is recursively enabled. Disabling a
  // heavyweight Container disables its children, whereas disabling
  // a lightweight Container does not.
  if (not dynamic_cast<Window*>(aComponent.get())) {
    for (auto enable_test = aComponent->get_parent(); enable_test; enable_test = enable_test->get_parent()) {
      if (not enable_test->is_enabled()) {
        return false;
      }

      if (dynamic_cast<Window*>(enable_test.get())) {
        break;
      }
    }
  }

  // TODO
//  auto focusable = aComponent->is_focusable();
//  if (aComponent->is_focus_traversable_overridden()) {
//    return focusable;
//  }

  return false;
}

}
