#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

namespace tui {

std::atomic<std::shared_ptr<Component>> KeyboardFocusManager::focus_owner;
std::atomic<std::shared_ptr<Window>> KeyboardFocusManager::focused_window;
std::atomic<std::shared_ptr<Component>> KeyboardFocusManager::current_focus_cycle_root;
std::atomic<std::shared_ptr<FocusTraversalPolicy>> KeyboardFocusManager::default_focus_traversal_policy;

void KeyboardFocusManager::clear_global_focus_owner() {
  std::shared_ptr<Window> active_window;
  if (active_window) {
    auto focus_owner = active_window->get_focus_owner();
//        if (focusLog.isLoggable(PlatformLogger.Level.FINE)) {
//            focusLog.fine("Clearing global focus owner " + focus_owner);
//        }
    if (focus_owner) {
//        FocusEvent fl = new FocusEvent(focus_owner, FocusEvent.FOCUS_LOST, false, null, FocusEvent.Cause.CLEAR_GLOBAL_FOCUS_OWNER);
//            SunToolkit.postPriorityEvent(fl);
    }
  }
}

}
