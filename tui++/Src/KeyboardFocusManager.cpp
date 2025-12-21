#include <tui++/Event.h>
#include <tui++/Frame.h>
#include <tui++/Dialog.h>
#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/DefaultFocusTraversalPolicy.h>

#include <tui++/util/log.h>

namespace tui {

KeyboardFocusManager::KeyboardFocusManager() :
    default_focus_traversal_policy(std::make_shared<DefaultFocusTraversalPolicy>()), //
    default_focus_traversal_keys { //
    make_shared_keystroke_set(KeyStroke { KeyEvent::VK_TAB, InputEvent::NO_MODIFIERS }), //
    make_shared_keystroke_set(KeyStroke { KeyEvent::VK_BACK_TAB, InputEvent::NO_MODIFIERS }), //
    nullptr, //
      nullptr } {
}

void KeyboardFocusManager::clear_focus_owner() {
  if (auto active_window = get_active_window()) {
    auto focus_owner = active_window->get_focus_owner();
    log_focus_ln("Clearing global focus owner " << focus_owner);
    if (focus_owner) {
//        FocusEvent fl = new FocusEvent(this->focus_owner, FocusEvent::FOCUS_LOST, false, null, FocusEvent::Cause::CLEAR_GLOBAL_FOCUS_OWNER);
//            SunToolkit.postPriorityEvent(fl);
    }
  }
}

void KeyboardFocusManager::set_most_recent_focus_owner(const std::shared_ptr<Component> &component) {
  auto parent = component;
  auto window = std::shared_ptr<Window> { };
  while (not (window = std::dynamic_pointer_cast<Window>(parent)) and parent) {
    parent = parent->get_parent();
  }
  if (window) {
    set_most_recent_focus_owner(window, component);
  }
}

void KeyboardFocusManager::set_most_recent_focus_owner(const std::shared_ptr<Window> &window, const std::shared_ptr<Component> &component) {
  this->most_recent_focus_owners[window] = component;
}

std::shared_ptr<Component> KeyboardFocusManager::get_most_recent_focus_owner(const std::shared_ptr<const Window> &window) {
  auto pos = this->most_recent_focus_owners.find(window);
  if (pos == this->most_recent_focus_owners.end()) {
    return {};
  }
  return pos->second.lock();
}

bool KeyboardFocusManager::request_focus(const std::shared_ptr<Component> &component, bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause) {
  auto current_focus_owner = this->focus_owner.load();
  if (component == current_focus_owner) {
    // Redundant request
    return true;
  }

  if (current_focus_owner) {
    auto current_focus_owner_event = make_event<FocusEvent>(current_focus_owner, FocusEvent::FOCUS_LOST, temporary, component);
    current_focus_owner_event.is_posted = true;
    current_focus_owner->dispatch_event(current_focus_owner_event);
  }

  auto new_focus_owner_event = make_event<FocusEvent>(component, FocusEvent::FOCUS_GAINED, temporary, current_focus_owner);
  new_focus_owner_event.is_posted = true;
  component->dispatch_event(new_focus_owner_event);
  return true;
}

static std::shared_ptr<Window> get_owning_frame_or_dialog(std::shared_ptr<Window> window) {
  while (window and not (dynamic_cast<Frame*>(window.get()) or dynamic_cast<Dialog*>(window.get()))) {
    window = std::dynamic_pointer_cast<Window>(window->get_parent());
  }
  return window;
}

bool KeyboardFocusManager::dispatch_event(Event &e) {
  log_focus_ln(e);

  switch (e.id) {
  case WindowEvent::WINDOW_GAINED_FOCUS: {
    auto &we = static_cast<WindowEvent&>(e);
    auto oldFocusedWindow = get_focused_window();
    auto newFocusedWindow = we.get_window();
    if (newFocusedWindow == oldFocusedWindow) {
      return false;
    }

    if (not (newFocusedWindow->is_focusable_window() and newFocusedWindow->is_visible() and newFocusedWindow->is_displayable())) {
      // we can not accept focus on such window, so reject it.
      restore_focus(we);
      break;
    }
    // If there exists a current focused window, then notify it
    // that it has lost focus.
    if (oldFocusedWindow) {
      send_event<WindowEvent>(oldFocusedWindow, WindowEvent::WINDOW_LOST_FOCUS, newFocusedWindow);
    }

    // Because the native libraries do not post WINDOW_ACTIVATED
    // events, we need to synthesize one if the active Window
    // changed.
    auto newActiveWindow = get_owning_frame_or_dialog(newFocusedWindow);
    auto currentActiveWindow = get_active_window();
    if (newActiveWindow != currentActiveWindow) {
      send_event<WindowEvent>(newActiveWindow, WindowEvent::WINDOW_ACTIVATED, currentActiveWindow);
      if (newActiveWindow != get_active_window()) {
        // Activation change was rejected. Unlikely, but possible.
        restore_focus(we);
        break;
      }
    }

    set_focused_window(newFocusedWindow);

    // Restore focus to the Component which last held it. We do
    // this here so that client code can override our choice in
    // a WINDOW_GAINED_FOCUS handler.
    //
    // Make sure that the focus change request doesn't change the
    // focused Window in case we are no longer the focused Window
    // when the request is handled.

    if (this->in_send_event == 0) {
      // Identify which Component should initially gain focus
      // in the Window.
      //
      // * If we're in SendMessage, then this is a synthetic
      //   WINDOW_GAINED_FOCUS message which was generated by a
      //   the FOCUS_GAINED handler. Allow the Component to
      //   which the FOCUS_GAINED message was targeted to
      //   receive the focus.
      // * Otherwise, look up the correct Component here.
      //   We don't use Window.getMostRecentFocusOwner because
      //   window is focused now and 'null' will be returned

      // Calculating of most recent focus owner and focus
      // request should be synchronized on KeyboardFocusManager.class
      // to prevent from thread race when user will request
      // focus between calculation and our request.
      // But if focus transfer is synchronous, this synchronization
      // may cause deadlock, thus we don't synchronize this block.
      auto to_focus = get_most_recent_focus_owner(newFocusedWindow);
      auto is_restore_focus_to = to_focus and to_focus == this->restore_focus_to.load();
      if (not to_focus and newFocusedWindow->is_focusable_window()) {
        to_focus = newFocusedWindow->get_focus_traversal_policy()->get_initial_component(newFocusedWindow);
      }

      auto temp_lost = newFocusedWindow->set_temporary_lost_component(nullptr);

      // The component which last has the focus when this window was focused
      // should receive focus first
      log_focus_ln("tempLost " << temp_lost << ", toFocus " << to_focus);
      if (temp_lost) {
        temp_lost->request_focus_in_window(is_restore_focus_to and temp_lost == to_focus ? FocusEvent::Cause::ROLLBACK : FocusEvent::Cause::ACTIVATION);
      }

      if (to_focus and to_focus != temp_lost) {
        // If there is a component which requested focus when this window
        // was inactive it expects to receive focus after activation.
        to_focus->request_focus_in_window(FocusEvent::Cause::ACTIVATION);
      }
    }

    this->restore_focus_to = nullptr;

    auto oppositeWindow = this->real_opposite_window.load().lock();
    if (oppositeWindow != we.opposite_window) {
      redispatch_event<WindowEvent>(newFocusedWindow, WindowEvent::WINDOW_GAINED_FOCUS, oppositeWindow);
    } else {
      redispatch_event(newFocusedWindow, e);
    }
    return true;
  }

  case WindowEvent::WINDOW_ACTIVATED: {
    auto &we = static_cast<WindowEvent&>(e);
    auto oldActiveWindow = get_active_window();
    auto newActiveWindow = we.get_window();
    if (oldActiveWindow == newActiveWindow) {
      break;
    }

    // If there exists a current active window, then notify it that
    // it has lost activation.
    if (oldActiveWindow) {
      send_event<WindowEvent>(oldActiveWindow, WindowEvent::WINDOW_DEACTIVATED, newActiveWindow);
      if (get_active_window()) {
        // Activation change was rejected. Unlikely, but possible.
        break;
      }
    }

    set_active_window(newActiveWindow);
    redispatch_event(newActiveWindow, e);
    return true;
  }

  case WindowEvent::WINDOW_DEACTIVATED: {
    auto currentActiveWindow = get_active_window();
    if (not currentActiveWindow) {
      break;
    }

    if (currentActiveWindow != e.source) {
      // The event is lost in time.
      // Allow listeners to precess the event but do not
      // change any global states
      break;
    }

    set_active_window(nullptr);
    e.source = currentActiveWindow;

    redispatch_event(currentActiveWindow, e);
    return true;
  }

  case WindowEvent::WINDOW_LOST_FOCUS: {
    auto &we = static_cast<WindowEvent&>(e);
    auto currentFocusedWindow = get_focused_window();
    auto losingFocusWindow = we.get_window();
    auto activeWindow = get_active_window();
    auto oppositeWindow = we.opposite_window;

    log_focus_ln("Active " << activeWindow << ", Current focused "<< currentFocusedWindow <<", losing focus " << losingFocusWindow << " opposite " << oppositeWindow);

    if (not currentFocusedWindow) {
      break;
    }

    // Special case -- if the native windowing system posts an
    // event claiming that the active Window has lost focus to the
    // focused Window, then discard the event. This is an artifact
    // of the native windowing system not knowing which Window is
    // really focused.
    if (this->in_send_event == 0 and losingFocusWindow == activeWindow and oppositeWindow == currentFocusedWindow) {
      break;
    }

    auto currentFocusOwner = get_focus_owner();
    if (currentFocusOwner) {
      // The focus owner should always receive a FOCUS_LOST event
      // before the Window is defocused.
      auto oppositeComp = std::shared_ptr<Component> { };
      if (oppositeWindow) {
        oppositeComp = oppositeWindow->get_temporary_lost_component();
        if (not oppositeComp) {
          oppositeComp = oppositeWindow->get_most_recent_focus_owner();
        }
      }
      if (not oppositeComp) {
        oppositeComp = oppositeWindow;
      }

      send_event<FocusEvent>(currentFocusOwner, FocusEvent::FOCUS_LOST, FocusEvent::Cause::ACTIVATION, true, oppositeComp);
    }

    set_focused_window(nullptr);

    e.source = currentFocusedWindow;

    if (oppositeWindow) {
      this->real_opposite_window = currentFocusedWindow;
    } else {
      this->real_opposite_window.store( { });
    }

    redispatch_event(currentFocusedWindow, e);

    if (not oppositeWindow and activeWindow) {
      // Then we need to deactivate the active Window as well.
      // No need to synthesize in other cases, because
      // WINDOW_ACTIVATED will handle it if necessary.
      send_event<WindowEvent>(activeWindow, WindowEvent::WINDOW_DEACTIVATED, nullptr);
    }
    break;
  }

  case FocusEvent::FOCUS_GAINED: {
    this->restore_focus_to = nullptr;
    auto &fe = static_cast<FocusEvent&>(e);
    auto oldFocusOwner = get_focus_owner();
    auto newFocusOwner = fe.component();
    if (oldFocusOwner == newFocusOwner) {
      log_focus_ln("Skipping " << fe << " because focus owner is the same");
      break;
    }

    // If there exists a current focus owner, then notify it that // it has lost focus.
    if (oldFocusOwner) {
      send_event<FocusEvent>(oldFocusOwner, FocusEvent::FOCUS_LOST, fe.cause, fe.temporary, newFocusOwner);
    }

    // Because the native windowing system has a different notion
    // of the current focus and activation states, it is possible
    // that a Component outside of the focused Window receives a
    // FOCUS_GAINED event. We synthesize a WINDOW_GAINED_FOCUS
    // event in that case.
    const auto newFocusedWindow = newFocusOwner->get_containing_window();
    const auto currentFocusedWindow = get_focused_window();

    if (newFocusedWindow and newFocusedWindow != currentFocusedWindow) {
      send_event<WindowEvent>(newFocusedWindow, WindowEvent::WINDOW_GAINED_FOCUS, currentFocusedWindow);
    }

    if (not (newFocusOwner->is_focusable() and newFocusOwner->is_showing() and
    // Refuse focus on a disabled component if the focus event
    // isn't of UNKNOWN reason (i.e. not a result of a direct request
    // but traversal, activation or system generated).
        (newFocusOwner->is_enabled() or fe.cause == FocusEvent::Cause::UNKNOWN))) {
      // we should not accept focus on such component, so reject it.
      // TODO
      if (false) { //is_auto_focus_transfer_enabled()) {
        // If FOCUS_GAINED is for a disposed component (however
        // it shouldn't happen) its toplevel parent is null. In this
        // case we have to try to restore focus in the current focused
        // window (for the details: 6607170).
        if (not newFocusedWindow) {
          restore_focus(fe, currentFocusedWindow);
        } else {
          restore_focus(fe, newFocusedWindow);
        }

        set_most_recent_focus_owner(newFocusedWindow, nullptr);
      }
      break;
    }

    set_focus_owner(newFocusOwner);

    if (not fe.temporary) {
      set_permanent_focus_owner(newFocusOwner);
    }

    auto oppositeComponent = this->real_opposite_component.load().lock();
    if (oppositeComponent and oppositeComponent != fe.opposite) {
      // TODO
//          ((AWTEvent) fe).isPosted = true;
      redispatch_event<FocusEvent>(newFocusOwner, FocusEvent::FOCUS_GAINED, fe.cause, fe.temporary, oppositeComponent);
    } else {
      redispatch_event(newFocusOwner, e);
    }
    return true;
  }

  case FocusEvent::FOCUS_LOST: {
    auto &fe = static_cast<FocusEvent&>(e);
    auto currentFocusOwner = get_focus_owner();
    if (not currentFocusOwner) {
      log_focus_ln("Skipping " << fe << " because focus owner is null");
      break;
    }

    // Ignore cases where a Component loses focus to itself.
    // If we make a mistake because of retargeting, then the
    // FOCUS_GAINED handler will correct it.
    if (currentFocusOwner == fe.opposite) {
      log_focus_ln("Skipping "<< fe << " because current focus owner is equal to opposite");
      break;
    }

    set_focus_owner(nullptr);

    if (not fe.temporary) {
      set_permanent_focus_owner(nullptr);
    } else if (auto owningWindow = currentFocusOwner->get_containing_window()) {
      owningWindow->set_temporary_lost_component(currentFocusOwner);
    }

    fe.source = currentFocusOwner;

    if (fe.opposite) {
      this->real_opposite_component = currentFocusOwner;
    } else {
      this->real_opposite_component.store( { });
    }

    redispatch_event(currentFocusOwner, fe);
    return true;
  }

  default:
    break;
  }

  return false;
}

void KeyboardFocusManager::focus_previous_component(const std::shared_ptr<Component> &component) {
  if (component) {
    component->transfer_focus_backward();
  }
}

void KeyboardFocusManager::focus_next_component(const std::shared_ptr<Component> &component) {
  if (component) {
    component->transfer_focus();
  }
}

void KeyboardFocusManager::up_focus_cycle(const std::shared_ptr<Component> &component) {
  if (component) {
    component->transfer_focus_up_cycle();
  }
}

void KeyboardFocusManager::down_focus_cycle(const std::shared_ptr<Component> &component) {
  if (component and component->is_focus_cycle_root()) {
    component->transfer_focus_down_cycle();
  }
}

void KeyboardFocusManager::process_key_event(const std::shared_ptr<Component> &focused_component, KeyEvent &e) {
// consume processed event if needed
  if (consume_processed_key_event(e)) {
    return;
  }

// KEY_TYPED events cannot be focus traversal keys
  if (e.id == KeyEvent::KEY_TYPED) {
    return;
  }

  if (focused_component->get_focus_traversal_keys_enabled() and not e.consumed) {
    auto stroke = KeyStroke { e };

    auto to_test = focused_component->get_focus_traversal_keys(KeyboardFocusManager::FORWARD_TRAVERSAL_KEYS);
    if (to_test->contains(stroke)) {
      consume_traversal_key(e);
      focus_next_component(focused_component);
      return;
    } else if (e.id == KeyEvent::KEY_PRESSED) {
      this->consume_next_key_typed = false;
    }

    to_test = focused_component->get_focus_traversal_keys(KeyboardFocusManager::BACKWARD_TRAVERSAL_KEYS);
    if (to_test->contains(stroke)) {
      consume_traversal_key(e);
      focus_previous_component(focused_component);
      return;
    }

    to_test = focused_component->get_focus_traversal_keys(KeyboardFocusManager::UP_CYCLE_TRAVERSAL_KEYS);
    if (to_test->contains(stroke)) {
      consume_traversal_key(e);
      up_focus_cycle(focused_component);
      return;
    }

    if (not focused_component->is_focus_cycle_root()) {
      return;
    }

    to_test = focused_component->get_focus_traversal_keys(KeyboardFocusManager::DOWN_CYCLE_TRAVERSAL_KEYS);
    if (to_test->contains(stroke)) {
      consume_traversal_key(e);
      down_focus_cycle(focused_component);
    }
  }
}

std::shared_ptr<const std::unordered_set<KeyStroke>> KeyboardFocusManager::get_default_focus_traversal_keys(FocusTraversalKeys id) {
  return this->default_focus_traversal_keys[id];
}

void KeyboardFocusManager::restore_focus(FocusEvent &e, const std::shared_ptr<Window> &new_focused_window) {
  auto opposite_component = this->real_opposite_component.load().lock();
  auto vetoed_component = e.component();

  if (new_focused_window and restore_focus(new_focused_window, vetoed_component, false)) {
  } else if (opposite_component and do_restore_focus(opposite_component, vetoed_component, false)) {
  } else if (e.opposite and do_restore_focus(e.opposite, vetoed_component, false)) {
  } else {
    clear_focus_owner();
  }
}

void KeyboardFocusManager::restore_focus(WindowEvent &e) {
  auto opposite_window = this->real_opposite_window.load().lock();
  if (opposite_window and restore_focus(opposite_window, nullptr, false)) {
    // do nothing, everything is done in restoreFocus()
  } else if (e.opposite_window and restore_focus(e.opposite_window, nullptr, false)) {
    // do nothing, everything is done in restoreFocus()
  } else {
    clear_focus_owner();
  }
}

bool KeyboardFocusManager::restore_focus(const std::shared_ptr<Window> &window, const std::shared_ptr<Component> &vetoed_component, bool clear_on_failure) {
  this->restore_focus_to = nullptr;
  auto to_focus = get_most_recent_focus_owner(window);
  if (to_focus and to_focus != vetoed_component) {
    if (do_restore_focus(to_focus, vetoed_component, false)) {
      return true;
    }
  }

  if (clear_on_failure) {
    clear_focus_owner();
    return true;
  } else {
    return false;
  }
}

bool KeyboardFocusManager::do_restore_focus(const std::shared_ptr<Component> &to_focus, const std::shared_ptr<Component> &vetoed_component, bool clear_on_failure) {
  auto success = true;
  if (to_focus != vetoed_component and to_focus->is_showing() and to_focus->can_be_focus_owner() and (success = to_focus->request_focus(false, FocusEvent::Cause::ROLLBACK))) {
    return true;
  } else {
    if (not success and get_focused_window() != to_focus->get_containing_window()) {
      this->restore_focus_to = to_focus;
      return true;
    }
    auto next_focus = to_focus->get_next_focus_candidate();
    if (next_focus and next_focus != vetoed_component and next_focus->request_focus_in_window(FocusEvent::Cause::ROLLBACK)) {
      return true;
    } else if (clear_on_failure) {
      clear_focus_owner();
      return true;
    } else {
      return false;
    }
  }
}

void KeyboardFocusManager::redispatch_event(const std::shared_ptr<Component> &target, Event &e) {
  e.is_being_dispatched_by_focus_manager = true;
  target->dispatch_event(e);
  e.is_being_dispatched_by_focus_manager = false;
}

void KeyboardFocusManager::clear_most_recent_focus_owner(const std::shared_ptr<Component> &c) {
  if (c) {
    auto window = c->with_tree_locked([&c]() {
      for (auto parent = c->get_parent(); parent; parent = parent->get_parent()) {
        if (auto window = std::dynamic_pointer_cast<Window>(parent)) {
          return window;
        }
      }
      return std::shared_ptr<Window> { };
    });

    if (window) {
      std::scoped_lock lock(this->mutex);
      if (get_most_recent_focus_owner(window) == c) {
        set_most_recent_focus_owner(window, nullptr);
      }
      // Also clear temporary lost component stored in Window
      if (window->get_temporary_lost_component() == c) {
        window->set_temporary_lost_component(nullptr);
      }
    }
  }
}

bool KeyboardFocusManager::is_auto_focus_transfer_enabled() {
  return true;
}

std::shared_ptr<Component> KeyboardFocusManager::get_permanent_focus_owner() {
  return this->permanent_focus_owner;
}

void KeyboardFocusManager::set_permanent_focus_owner(std::shared_ptr<Component> const &c) {
  if (not c or c->is_focusable()) {
    this->permanent_focus_owner = c;
    set_most_recent_focus_owner(c);
  }
}

}
