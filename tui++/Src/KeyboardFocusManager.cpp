#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

namespace tui {

std::atomic<std::shared_ptr<Component>> KeyboardFocusManager::focus_owner;
std::atomic<std::shared_ptr<Window>> KeyboardFocusManager::focused_window;
std::atomic<std::shared_ptr<Component>> KeyboardFocusManager::current_focus_cycle_root;
std::atomic<std::shared_ptr<FocusTraversalPolicy>> KeyboardFocusManager::default_focus_traversal_policy;
std::map<std::shared_ptr<Window>, std::weak_ptr<Component>> KeyboardFocusManager::most_recent_focus_owners;

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
  most_recent_focus_owners[window] = component;
}

std::shared_ptr<Component> KeyboardFocusManager::get_most_recent_focus_owner(const std::shared_ptr<Window> &window) {
  auto pos = most_recent_focus_owners.find(window);
  if (pos == most_recent_focus_owners.end()) {
    return {};
  }
  return pos->second.lock();
}

bool KeyboardFocusManager::request_focus(const std::shared_ptr<Component> &component, bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause) {
  auto current_focus_owner = focus_owner.load();
  if (component == current_focus_owner) {
    // Redundant request
    return true;
  }

  if (current_focus_owner) {
    auto current_focus_owner_event = std::make_shared<Event>(std::in_place_type<FocusEvent>, current_focus_owner, FocusEvent::FOCUS_LOST, temporary, component);
//    current_focus_owner_event->is_posted = true;
    current_focus_owner->dispatch_event(current_focus_owner_event);
  }

  auto new_focus_owner_event = std::make_shared<Event>(std::in_place_type<FocusEvent>, component, FocusEvent::FOCUS_GAINED, temporary, current_focus_owner);
//  new_focus_owner_event->is_posted = true;
  component->dispatch_event(new_focus_owner_event);
  return true;
}

bool KeyboardFocusManager::dispatch_event(const std::shared_ptr<Event> &e) {
//  public boolean dispatchEvent(AWTEvent e) {
//      if (focusLog.isLoggable(PlatformLogger.Level.FINE) && (e instanceof WindowEvent || e instanceof FocusEvent)) {
//          focusLog.fine("" + e);
//      }
//      switch (e.getID()) {
//          case WindowEvent.WINDOW_GAINED_FOCUS: {
//              if (repostIfFollowsKeyEvents((WindowEvent)e)) {
//                  break;
//              }
//
//              WindowEvent we = (WindowEvent)e;
//              Window oldFocusedWindow = getGlobalFocusedWindow();
//              Window newFocusedWindow = we.getWindow();
//              if (newFocusedWindow == oldFocusedWindow) {
//                  break;
//              }
//
//              if (!(newFocusedWindow.isFocusableWindow()
//                    && newFocusedWindow.isVisible()
//                    && newFocusedWindow.isDisplayable()))
//              {
//                  // we can not accept focus on such window, so reject it.
//                  restoreFocus(we);
//                  break;
//              }
//              // If there exists a current focused window, then notify it
//              // that it has lost focus.
//              if (oldFocusedWindow != null) {
//                  boolean isEventDispatched =
//                      sendMessage(oldFocusedWindow,
//                              new WindowEvent(oldFocusedWindow,
//                                              WindowEvent.WINDOW_LOST_FOCUS,
//                                              newFocusedWindow));
//                  // Failed to dispatch, clear by ourselves
//                  if (!isEventDispatched) {
//                      setGlobalFocusOwner(null);
//                      setGlobalFocusedWindow(null);
//                  }
//              }
//
//              // Because the native libraries do not post WINDOW_ACTIVATED
//              // events, we need to synthesize one if the active Window
//              // changed.
//              Window newActiveWindow =
//                  getOwningFrameDialog(newFocusedWindow);
//              Window currentActiveWindow = getGlobalActiveWindow();
//              if (newActiveWindow != currentActiveWindow) {
//                  sendMessage(newActiveWindow,
//                              new WindowEvent(newActiveWindow,
//                                              WindowEvent.WINDOW_ACTIVATED,
//                                              currentActiveWindow));
//                  if (newActiveWindow != getGlobalActiveWindow()) {
//                      // Activation change was rejected. Unlikely, but
//                      // possible.
//                      restoreFocus(we);
//                      break;
//                  }
//              }
//
//              setGlobalFocusedWindow(newFocusedWindow);
//
//              if (newFocusedWindow != getGlobalFocusedWindow()) {
//                  // Focus change was rejected. Will happen if
//                  // newFocusedWindow is not a focusable Window.
//                  restoreFocus(we);
//                  break;
//              }
//
//              // Restore focus to the Component which last held it. We do
//              // this here so that client code can override our choice in
//              // a WINDOW_GAINED_FOCUS handler.
//              //
//              // Make sure that the focus change request doesn't change the
//              // focused Window in case we are no longer the focused Window
//              // when the request is handled.
//              if (inSendMessage == 0) {
//                  // Identify which Component should initially gain focus
//                  // in the Window.
//                  //
//                  // * If we're in SendMessage, then this is a synthetic
//                  //   WINDOW_GAINED_FOCUS message which was generated by a
//                  //   the FOCUS_GAINED handler. Allow the Component to
//                  //   which the FOCUS_GAINED message was targeted to
//                  //   receive the focus.
//                  // * Otherwise, look up the correct Component here.
//                  //   We don't use Window.getMostRecentFocusOwner because
//                  //   window is focused now and 'null' will be returned
//
//
//                  // Calculating of most recent focus owner and focus
//                  // request should be synchronized on KeyboardFocusManager.class
//                  // to prevent from thread race when user will request
//                  // focus between calculation and our request.
//                  // But if focus transfer is synchronous, this synchronization
//                  // may cause deadlock, thus we don't synchronize this block.
//                  Component toFocus = KeyboardFocusManager.
//                      getMostRecentFocusOwner(newFocusedWindow);
//                  boolean isFocusRestore = restoreFocusTo != null &&
//                                                    toFocus == restoreFocusTo;
//                  if ((toFocus == null) &&
//                      newFocusedWindow.isFocusableWindow())
//                  {
//                      toFocus = newFocusedWindow.getFocusTraversalPolicy().
//                          getInitialComponent(newFocusedWindow);
//                  }
//                  Component tempLost = null;
//                  synchronized(KeyboardFocusManager.class) {
//                      tempLost = newFocusedWindow.setTemporaryLostComponent(null);
//                  }
//
//                  // The component which last has the focus when this window was focused
//                  // should receive focus first
//                  if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//                      focusLog.finer("tempLost {0}, toFocus {1}",
//                                     tempLost, toFocus);
//                  }
//                  if (tempLost != null) {
//                      tempLost.requestFocusInWindow(
//                                  isFocusRestore && tempLost == toFocus ?
//                                              FocusEvent.Cause.ROLLBACK :
//                                              FocusEvent.Cause.ACTIVATION);
//                  }
//
//                  if (toFocus != null && toFocus != tempLost) {
//                      // If there is a component which requested focus when this window
//                      // was inactive it expects to receive focus after activation.
//                      toFocus.requestFocusInWindow(FocusEvent.Cause.ACTIVATION);
//                  }
//              }
//              restoreFocusTo = null;
//
//              Window realOppositeWindow = this.realOppositeWindowWR.get();
//              if (realOppositeWindow != we.getOppositeWindow()) {
//                  we = new WindowEvent(newFocusedWindow,
//                                       WindowEvent.WINDOW_GAINED_FOCUS,
//                                       realOppositeWindow);
//              }
//              return typeAheadAssertions(newFocusedWindow, we);
//          }
//
//          case WindowEvent.WINDOW_ACTIVATED: {
//              WindowEvent we = (WindowEvent)e;
//              Window oldActiveWindow = getGlobalActiveWindow();
//              Window newActiveWindow = we.getWindow();
//              if (oldActiveWindow == newActiveWindow) {
//                  break;
//              }
//
//              // If there exists a current active window, then notify it that
//              // it has lost activation.
//              if (oldActiveWindow != null) {
//                  boolean isEventDispatched =
//                      sendMessage(oldActiveWindow,
//                              new WindowEvent(oldActiveWindow,
//                                              WindowEvent.WINDOW_DEACTIVATED,
//                                              newActiveWindow));
//                  // Failed to dispatch, clear by ourselves
//                  if (!isEventDispatched) {
//                      setGlobalActiveWindow(null);
//                  }
//                  if (getGlobalActiveWindow() != null) {
//                      // Activation change was rejected. Unlikely, but
//                      // possible.
//                      break;
//                  }
//              }
//
//              setGlobalActiveWindow(newActiveWindow);
//
//              if (newActiveWindow != getGlobalActiveWindow()) {
//                  // Activation change was rejected. Unlikely, but
//                  // possible.
//                  break;
//              }
//
//              return typeAheadAssertions(newActiveWindow, we);
//          }
//
//          case FocusEvent.FOCUS_GAINED: {
//              restoreFocusTo = null;
//              FocusEvent fe = (FocusEvent)e;
//              Component oldFocusOwner = getGlobalFocusOwner();
//              Component newFocusOwner = fe.getComponent();
//              if (oldFocusOwner == newFocusOwner) {
//                  if (focusLog.isLoggable(PlatformLogger.Level.FINE)) {
//                      focusLog.fine("Skipping {0} because focus owner is the same", e);
//                  }
//                  // We can't just drop the event - there could be
//                  // type-ahead markers associated with it.
//                  dequeueKeyEvents(-1, newFocusOwner);
//                  break;
//              }
//
//              // If there exists a current focus owner, then notify it that
//              // it has lost focus.
//              if (oldFocusOwner != null) {
//                  boolean isEventDispatched =
//                      sendMessage(oldFocusOwner,
//                                  new FocusEvent(oldFocusOwner,
//                                                 FocusEvent.FOCUS_LOST,
//                                                 fe.isTemporary(),
//                                                 newFocusOwner, fe.getCause()));
//                  // Failed to dispatch, clear by ourselves
//                  if (!isEventDispatched) {
//                      setGlobalFocusOwner(null);
//                      if (!fe.isTemporary()) {
//                          setGlobalPermanentFocusOwner(null);
//                      }
//                  }
//              }
//
//              // Because the native windowing system has a different notion
//              // of the current focus and activation states, it is possible
//              // that a Component outside of the focused Window receives a
//              // FOCUS_GAINED event. We synthesize a WINDOW_GAINED_FOCUS
//              // event in that case.
//              final Window newFocusedWindow = SunToolkit.getContainingWindow(newFocusOwner);
//              final Window currentFocusedWindow = getGlobalFocusedWindow();
//              if (newFocusedWindow != null &&
//                  newFocusedWindow != currentFocusedWindow)
//              {
//                  sendMessage(newFocusedWindow,
//                              new WindowEvent(newFocusedWindow,
//                                      WindowEvent.WINDOW_GAINED_FOCUS,
//                                              currentFocusedWindow));
//                  if (newFocusedWindow != getGlobalFocusedWindow()) {
//                      // Focus change was rejected. Will happen if
//                      // newFocusedWindow is not a focusable Window.
//
//                      // Need to recover type-ahead, but don't bother
//                      // restoring focus. That was done by the
//                      // WINDOW_GAINED_FOCUS handler
//                      dequeueKeyEvents(-1, newFocusOwner);
//                      break;
//                  }
//              }
//
//              if (!(newFocusOwner.isFocusable() && newFocusOwner.isShowing() &&
//                  // Refuse focus on a disabled component if the focus event
//                  // isn't of UNKNOWN reason (i.e. not a result of a direct request
//                  // but traversal, activation or system generated).
//                  (newFocusOwner.isEnabled() || fe.getCause().equals(FocusEvent.Cause.UNKNOWN))))
//              {
//                  // we should not accept focus on such component, so reject it.
//                  dequeueKeyEvents(-1, newFocusOwner);
//                  if (KeyboardFocusManager.isAutoFocusTransferEnabled()) {
//                      // If FOCUS_GAINED is for a disposed component (however
//                      // it shouldn't happen) its toplevel parent is null. In this
//                      // case we have to try to restore focus in the current focused
//                      // window (for the details: 6607170).
//                      if (newFocusedWindow == null) {
//                          restoreFocus(fe, currentFocusedWindow);
//                      } else {
//                          restoreFocus(fe, newFocusedWindow);
//                      }
//                      setMostRecentFocusOwner(newFocusedWindow, null); // see: 8013773
//                  }
//                  break;
//              }
//
//              setGlobalFocusOwner(newFocusOwner);
//
//              if (newFocusOwner != getGlobalFocusOwner()) {
//                  // Focus change was rejected. Will happen if
//                  // newFocusOwner is not focus traversable.
//                  dequeueKeyEvents(-1, newFocusOwner);
//                  if (KeyboardFocusManager.isAutoFocusTransferEnabled()) {
//                      restoreFocus(fe, newFocusedWindow);
//                  }
//                  break;
//              }
//
//              if (!fe.isTemporary()) {
//                  setGlobalPermanentFocusOwner(newFocusOwner);
//
//                  if (newFocusOwner != getGlobalPermanentFocusOwner()) {
//                      // Focus change was rejected. Unlikely, but possible.
//                      dequeueKeyEvents(-1, newFocusOwner);
//                      if (KeyboardFocusManager.isAutoFocusTransferEnabled()) {
//                          restoreFocus(fe, newFocusedWindow);
//                      }
//                      break;
//                  }
//              }
//
//              setNativeFocusOwner(getHeavyweight(newFocusOwner));
//
//              Component realOppositeComponent = this.realOppositeComponentWR.get();
//              if (realOppositeComponent != null &&
//                  realOppositeComponent != fe.getOppositeComponent()) {
//                  fe = new FocusEvent(newFocusOwner,
//                                      FocusEvent.FOCUS_GAINED,
//                                      fe.isTemporary(),
//                                      realOppositeComponent, fe.getCause());
//                  ((AWTEvent) fe).isPosted = true;
//              }
//              return typeAheadAssertions(newFocusOwner, fe);
//          }
//
//          case FocusEvent.FOCUS_LOST: {
//              FocusEvent fe = (FocusEvent)e;
//              Component currentFocusOwner = getGlobalFocusOwner();
//              if (currentFocusOwner == null) {
//                  if (focusLog.isLoggable(PlatformLogger.Level.FINE))
//                      focusLog.fine("Skipping {0} because focus owner is null", e);
//                  break;
//              }
//              // Ignore cases where a Component loses focus to itself.
//              // If we make a mistake because of retargeting, then the
//              // FOCUS_GAINED handler will correct it.
//              if (currentFocusOwner == fe.getOppositeComponent()) {
//                  if (focusLog.isLoggable(PlatformLogger.Level.FINE))
//                      focusLog.fine("Skipping {0} because current focus owner is equal to opposite", e);
//                  break;
//              }
//
//              setGlobalFocusOwner(null);
//
//              if (getGlobalFocusOwner() != null) {
//                  // Focus change was rejected. Unlikely, but possible.
//                  restoreFocus(currentFocusOwner, true);
//                  break;
//              }
//
//              if (!fe.isTemporary()) {
//                  setGlobalPermanentFocusOwner(null);
//
//                  if (getGlobalPermanentFocusOwner() != null) {
//                      // Focus change was rejected. Unlikely, but possible.
//                      restoreFocus(currentFocusOwner, true);
//                      break;
//                  }
//              } else {
//                  Window owningWindow = currentFocusOwner.getContainingWindow();
//                  if (owningWindow != null) {
//                      owningWindow.setTemporaryLostComponent(currentFocusOwner);
//                  }
//              }
//
//              setNativeFocusOwner(null);
//
//              fe.setSource(currentFocusOwner);
//
//              realOppositeComponentWR = (fe.getOppositeComponent() != null)
//                  ? new WeakReference<Component>(currentFocusOwner)
//                  : NULL_COMPONENT_WR;
//
//              return typeAheadAssertions(currentFocusOwner, fe);
//          }
//
//          case WindowEvent.WINDOW_DEACTIVATED: {
//              WindowEvent we = (WindowEvent)e;
//              Window currentActiveWindow = getGlobalActiveWindow();
//              if (currentActiveWindow == null) {
//                  break;
//              }
//
//              if (currentActiveWindow != e.getSource()) {
//                  // The event is lost in time.
//                  // Allow listeners to precess the event but do not
//                  // change any global states
//                  break;
//              }
//
//              setGlobalActiveWindow(null);
//              if (getGlobalActiveWindow() != null) {
//                  // Activation change was rejected. Unlikely, but possible.
//                  break;
//              }
//
//              we.setSource(currentActiveWindow);
//              return typeAheadAssertions(currentActiveWindow, we);
//          }
//
//          case WindowEvent.WINDOW_LOST_FOCUS: {
//              if (repostIfFollowsKeyEvents((WindowEvent)e)) {
//                  break;
//              }
//
//              WindowEvent we = (WindowEvent)e;
//              Window currentFocusedWindow = getGlobalFocusedWindow();
//              Window losingFocusWindow = we.getWindow();
//              Window activeWindow = getGlobalActiveWindow();
//              Window oppositeWindow = we.getOppositeWindow();
//              if (focusLog.isLoggable(PlatformLogger.Level.FINE))
//                  focusLog.fine("Active {0}, Current focused {1}, losing focus {2} opposite {3}",
//                                activeWindow, currentFocusedWindow,
//                                losingFocusWindow, oppositeWindow);
//              if (currentFocusedWindow == null) {
//                  break;
//              }
//
//              // Special case -- if the native windowing system posts an
//              // event claiming that the active Window has lost focus to the
//              // focused Window, then discard the event. This is an artifact
//              // of the native windowing system not knowing which Window is
//              // really focused.
//              if (inSendMessage == 0 && losingFocusWindow == activeWindow &&
//                  oppositeWindow == currentFocusedWindow)
//              {
//                  break;
//              }
//
//              Component currentFocusOwner = getGlobalFocusOwner();
//              if (currentFocusOwner != null) {
//                  // The focus owner should always receive a FOCUS_LOST event
//                  // before the Window is defocused.
//                  Component oppositeComp = null;
//                  if (oppositeWindow != null) {
//                      oppositeComp = oppositeWindow.getTemporaryLostComponent();
//                      if (oppositeComp == null) {
//                          oppositeComp = oppositeWindow.getMostRecentFocusOwner();
//                      }
//                  }
//                  if (oppositeComp == null) {
//                      oppositeComp = oppositeWindow;
//                  }
//                  sendMessage(currentFocusOwner,
//                              new FocusEvent(currentFocusOwner,
//                                             FocusEvent.FOCUS_LOST,
//                                             true,
//                                             oppositeComp, FocusEvent.Cause.ACTIVATION));
//              }
//
//              setGlobalFocusedWindow(null);
//              if (getGlobalFocusedWindow() != null) {
//                  // Focus change was rejected. Unlikely, but possible.
//                  restoreFocus(currentFocusedWindow, null, true);
//                  break;
//              }
//
//              we.setSource(currentFocusedWindow);
//              realOppositeWindowWR = (oppositeWindow != null)
//                  ? new WeakReference<Window>(currentFocusedWindow)
//                  : NULL_WINDOW_WR;
//              typeAheadAssertions(currentFocusedWindow, we);
//
//              if (oppositeWindow == null && activeWindow != null) {
//                  // Then we need to deactivate the active Window as well.
//                  // No need to synthesize in other cases, because
//                  // WINDOW_ACTIVATED will handle it if necessary.
//                  sendMessage(activeWindow,
//                              new WindowEvent(activeWindow,
//                                              WindowEvent.WINDOW_DEACTIVATED,
//                                              null));
//                  if (getGlobalActiveWindow() != null) {
//                      // Activation change was rejected. Unlikely,
//                      // but possible.
//                      restoreFocus(currentFocusedWindow, null, true);
//                  }
//              }
//              break;
//          }
//
//          case KeyEvent.KEY_TYPED:
//          case KeyEvent.KEY_PRESSED:
//          case KeyEvent.KEY_RELEASED:
//              return typeAheadAssertions(null, e);
//
//          default:
//              return false;
//      }
//
//      return true;
//  }
}

void KeyboardFocusManager::process_key_event(const std::shared_ptr<Component> &focused_component, KeyEvent &e) {
//  // consume processed event if needed
//  if (consumeProcessedKeyEvent(e)) {
//      return;
//  }
//
//  // KEY_TYPED events cannot be focus traversal keys
//  if (e.getID() == KeyEvent.KEY_TYPED) {
//      return;
//  }
//
//  if (focusedComponent.getFocusTraversalKeysEnabled() &&
//      !e.isConsumed())
//  {
//      AWTKeyStroke stroke = AWTKeyStroke.getAWTKeyStrokeForEvent(e),
//          oppStroke = AWTKeyStroke.getAWTKeyStroke(stroke.getKeyCode(),
//                                           stroke.getModifiers(),
//                                           !stroke.isOnKeyRelease());
//      Set<AWTKeyStroke> toTest;
//      boolean contains, containsOpp;
//
//      toTest = focusedComponent.getFocusTraversalKeys(
//          KeyboardFocusManager.FORWARD_TRAVERSAL_KEYS);
//      contains = toTest.contains(stroke);
//      containsOpp = toTest.contains(oppStroke);
//
//      if (contains || containsOpp) {
//          consumeTraversalKey(e);
//          if (contains) {
//              focusNextComponent(focusedComponent);
//          }
//          return;
//      } else if (e.getID() == KeyEvent.KEY_PRESSED) {
//          // Fix for 6637607: consumeNextKeyTyped should be reset.
//          consumeNextKeyTyped = false;
//      }
//
//      toTest = focusedComponent.getFocusTraversalKeys(
//          KeyboardFocusManager.BACKWARD_TRAVERSAL_KEYS);
//      contains = toTest.contains(stroke);
//      containsOpp = toTest.contains(oppStroke);
//
//      if (contains || containsOpp) {
//          consumeTraversalKey(e);
//          if (contains) {
//              focusPreviousComponent(focusedComponent);
//          }
//          return;
//      }
//
//      toTest = focusedComponent.getFocusTraversalKeys(
//          KeyboardFocusManager.UP_CYCLE_TRAVERSAL_KEYS);
//      contains = toTest.contains(stroke);
//      containsOpp = toTest.contains(oppStroke);
//
//      if (contains || containsOpp) {
//          consumeTraversalKey(e);
//          if (contains) {
//              upFocusCycle(focusedComponent);
//          }
//          return;
//      }
//
//      if (!((focusedComponent instanceof Container) &&
//            ((Container)focusedComponent).isFocusCycleRoot())) {
//          return;
//      }
//
//      toTest = focusedComponent.getFocusTraversalKeys(
//          KeyboardFocusManager.DOWN_CYCLE_TRAVERSAL_KEYS);
//      contains = toTest.contains(stroke);
//      containsOpp = toTest.contains(oppStroke);
//
//      if (contains || containsOpp) {
//          consumeTraversalKey(e);
//          if (contains) {
//              downFocusCycle((Container)focusedComponent);
//          }
//      }
//  }

}

}
