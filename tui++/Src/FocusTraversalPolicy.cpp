#include <tui++/FocusTraversalPolicy.h>
#include <tui++/Window.h>

#include <stdexcept>

namespace tui {

std::shared_ptr<Component> FocusTraversalPolicy::get_initial_component(const std::shared_ptr<Window> &window) const {
  if (not window) {
    throw std::runtime_error("window cannot be equal to null.");
  }

  auto def = get_default_component(window);
  if (not def and window->is_focusable_window()) {
    return window;
  }
  return def;
}

}
