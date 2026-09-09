#include <tui++/DefaultFocusTraversalPolicy.h>

#include <tui++/Component.h>
#include <tui++/LayeredPane.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/RootPane.h>
#include <tui++/ScrollPane.h>
#include <tui++/Viewport.h>
#include <tui++/Window.h>

namespace tui {

bool DefaultFocusTraversalPolicy::accept(const std::shared_ptr<Component> &candidate) const {
  // Swing's DefaultFocusTraversalPolicy: the new focus owner must be
  // showing, displayable, enabled and focusable, and (the heavyweight
  // container rule) every ancestor up to the window must be enabled.
  if (not candidate->can_be_focus_owner()) {
    return false;
  }

  // A window itself is never a traversal stop; the cycle walks its
  // children.
  if (is_a<Window>(candidate)) {
    return false;
  }

  // Swing's LayoutFocusTraversalPolicy does not accept menu items (a menu
  // bar is reached with F10/Alt+mnemonic, never with Tab) ...
  if (is_a<MenuItem>(candidate) or is_a<MenuBar>(candidate)) {
    return false;
  }

  // ... and the structural containers this toolkit keeps focusable for the
  // API never take Tab focus either: they do not consume keyboard input of
  // their own, and every one of them (root/layered pane, panel, scroll
  // pane, viewport) would otherwise stand before the control inside it.
  if (is_a<RootPane>(candidate) or is_a<LayeredPane>(candidate) or is_a<Panel>(candidate) or is_a<ScrollPane>(candidate) or is_a<Viewport>(candidate)) {
    return false;
  }

  for (auto parent = candidate->get_parent(); parent; parent = parent->get_parent()) {
    if (not parent->is_enabled()) {
      return false;
    }

    if (is_a<Window>(parent)) {
      break;
    }
  }

  return true;
}

}
