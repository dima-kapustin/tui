#include <tui++/Popup.h>
#include <tui++/Border.h>
#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/Component.h>
#include <tui++/KeyStroke.h>
#include <tui++/KeyboardManager.h>

#include <tui++/Screen.h>

#include <tui++/util/log.h>

namespace tui {

std::recursive_mutex Component::tree_mutex;
bool Component::descend_unconditionally_when_validating = false;

Component::~Component() {
}

void Component::paint_border(Graphics &g) {
  if (this->border) {
    this->border->paint_border(*this, g, 0, 0, get_width(), get_height());
  }
}

void Component::paint_components(Graphics &g) {
  for (auto &&c : this->components) {
    if (c->is_visible()) {
      int x = c->get_x(), y = c->get_y();
      g.translate(x, y);
      auto clip_rect = g.get_clip_rect();
      g.clip_rect(0, 0, c->get_width(), c->get_height());
      c->paint(g);
      g.set_clip_rect(clip_rect);
      g.translate(-x, -y);
    }
  }
}

void Component::enable_events(EventTypeMask event_mask) {
  this->event_mask |= event_mask;

  if (auto window = get_containing_window()) {
    window->enable_events_for_dispatching(event_mask);
  }
}

void Component::disable_events(EventTypeMask event_mask) {
  this->event_mask &= ~event_mask;
}

bool Component::is_request_focus_accepted(bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause) {
  if (not is_focusable() or not is_visible()) {
    // TODO
//    if (focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//      focusLog.finest("Not focusable or not visible");
//    }
    return false;
  }

  auto window = get_containing_window();
  if (not window or not window->is_focusable_window()) {
    // TODO
//    if (focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//      focusLog.finest("Component doesn't have toplevel");
//    }
    return false;
  }

  // We have passed all regular checks for focus request,
  // now let's call RequestFocusController and see what it says.
  auto focus_owner = KeyboardFocusManager::get_most_recent_focus_owner(window);
  if (not focus_owner) {
    // sometimes most recent focus owner may be null, but focus owner is not
    // e.g. we reset most recent focus owner if user removes focus owner
    focus_owner = KeyboardFocusManager::get_focus_owner();
    if (focus_owner and focus_owner->get_containing_window() != window) {
      focus_owner = nullptr;
    }
  }

  if (focus_owner.get() == this or not focus_owner) {
    // Controller is supposed to verify focus transfers and for this it
    // should know both from and to components.  And it shouldn't verify
    // transfers from when these components are equal.

    // TODO
//    if (focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//      focusLog.finest("focus owner is null or this");
//    }
    return true;
  }

  if (cause == FocusEvent::Cause::ACTIVATION) {
    // we shouldn't call RequestFocusController in case we are
    // in activation.  We do request focus on component which
    // has got temporary focus lost and then on component which is
    // most recent focus owner.  But most recent focus owner can be
    // changed by requestFocusXXX() call only, so this transfer has
    // been already approved.
    // TODO
//    if (focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//      focusLog.finest("cause is activation");
//    }
    return true;
  }

  // TODO
//  bool ret = Component.requestFocusController.acceptRequestFocus(focus_owner, this, temporary, focusedWindowChangeAllowed, cause);
//  if (focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//    focusLog.finest("RequestFocusController returns {0}", ret);
//  }
//
//  return ret;
  return true;
}

bool Component::request_focus(bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause) {
  // 1) Check if the event being dispatched is a system-generated mouse event.
  if (auto current_event = get_event_queue()->get_current_event(); current_event and current_event->system_generated) {
    if (auto mouse_event = std::dynamic_pointer_cast<MouseEvent>(current_event)) {
      // 2) Sanity check: if the mouse event component source belongs to the same containing window.
      auto source = mouse_event->source;
      if (not source or source->get_containing_window() == get_containing_window()) {
        log_focus_ln("requesting focus by mouse event \"in window\"");

        // If both the conditions are fulfilled the focus request should be strictly
        // bounded by the toplevel window. It's assumed that the mouse event activates
        // the window (if it wasn't active) and this makes it possible for a focus
        // request with a strong in-window requirement to change focus in the bounds
        // of the toplevel. If, by any means, due to asynchronous nature of the event
        // dispatching mechanism, the window happens to be natively inactive by the time
        // this focus request is eventually handled, it should not re-activate the
        // toplevel. Otherwise the result may not meet user expectations. See 6981400.
        focused_window_change_allowed = false;
      }
    }
  }

  if (not is_request_focus_accepted(temporary, focused_window_change_allowed, cause)) {
    // TODO
//    if (focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//      focusLog.finest("requestFocus is not accepted");
//    }
    return false;
  }

  // Update most-recent map
  KeyboardFocusManager::set_most_recent_focus_owner(shared_from_this());

  auto window = shared_from_this();
  while (not std::dynamic_pointer_cast<Window>(window)) {
    if (not window->is_visible()) {
      // TODO
//      if (focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//        focusLog.finest("component is recursively invisible");
//      }
      return false;
    }
    window = window->get_parent();
  }

  // Focus this Component
  return KeyboardFocusManager::request_focus(shared_from_this(), temporary, focused_window_change_allowed, cause);
}

std::shared_ptr<Component> Component::get_next_focus_candidate() const {
  auto root_ancestor = get_traversal_root();
  auto comp = const_cast<Component*>(this)->shared_from_this();
  while (root_ancestor and not (root_ancestor->is_showing() and root_ancestor->can_be_focus_owner())) {
    comp = root_ancestor;
    root_ancestor = comp->get_focus_cycle_root_ancestor();
  }

  // TODO
//    if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//        focusLog.finer("comp = " + comp + ", root = " + rootAncestor);
//    }

  std::shared_ptr<Component> candidate;
  if (root_ancestor) {
    auto policy = root_ancestor->get_focus_traversal_policy();
    auto to_focus = policy->get_component_after(root_ancestor, comp);
    // TODO
//      if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//        focusLog.finer("component after is " + toFocus);
//      }
    if (not to_focus) {
      to_focus = policy->get_default_component(root_ancestor);
      // TODO
//        if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//          focusLog.finer("default component is " + toFocus);
//        }
    }
    candidate = to_focus;
  }
  // TODO
//    if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//      focusLog.finer("Focus transfer candidate: " + candidate);
//    }
  return candidate;
}

bool Component::transfer_focus(bool clear_on_failure) {
  // TODO
//  if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//      focusLog.finer("clearOnFailure = " + clearOnFailure);
//  }
  auto to_focus = get_next_focus_candidate();
  auto result = false;
  if (to_focus and not to_focus->is_focus_owner() and to_focus != shared_from_this()) {
//      res = toFocus.request_focus_in_window(FocusEvent::Cause::TRAVERSAL_FORWARD);
  }
  if (clear_on_failure and not result) {
    // TODO
//      if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//          focusLog.finer("clear global focus owner");
//      }
    KeyboardFocusManager::clear_focus_owner();
  }
  // TODO
//  if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//      focusLog.finer("returning result: " + res);
//  }
  return result;
}

bool Component::transfer_focus_backward(bool clear_on_failure) {
  auto root_ancestor = get_traversal_root();
  auto comp = shared_from_this();
  while (root_ancestor and not (root_ancestor->is_showing() and root_ancestor->can_be_focus_owner())) {
    comp = root_ancestor;
    root_ancestor = comp->get_focus_cycle_root_ancestor();
  }
  auto result = false;
  if (root_ancestor) {
    auto policy = root_ancestor->get_focus_traversal_policy();
    auto to_focus = policy->get_component_before(root_ancestor, comp);
    if (not to_focus) {
      to_focus = policy->get_default_component(root_ancestor);
    }
    if (to_focus) {
      result = to_focus->request_focus_in_window(FocusEvent::Cause::TRAVERSAL_BACKWARD);
    }
  }
  if (not result and clear_on_failure) {
    log_focus_ln("clear global focus owner");
    KeyboardFocusManager::clear_focus_owner();
  }
//  log_focus_ln("returning result: " << result);
  return result;
}

void Component::transfer_focus_up_cycle() {
  auto root_ancestor = get_focus_cycle_root_ancestor();
  while (root_ancestor and not (root_ancestor->is_showing() and root_ancestor->is_focusable() and root_ancestor->is_enabled())) {
    root_ancestor = root_ancestor->get_focus_cycle_root_ancestor();
  }

  if (root_ancestor) {
    auto root_ancestor_root_ancestor = root_ancestor->get_focus_cycle_root_ancestor();
    auto fcr = root_ancestor_root_ancestor ? root_ancestor_root_ancestor : root_ancestor;

    KeyboardFocusManager::get_current_focus_cycle_root(fcr);
    root_ancestor->request_focus(FocusEvent::Cause::TRAVERSAL_UP);
  } else {
    if (auto window = get_containing_window()) {
      if (auto to_focus = window->get_focus_traversal_policy()->get_default_component(window)) {
        KeyboardFocusManager::get_current_focus_cycle_root(window);
        to_focus->request_focus(FocusEvent::Cause::TRAVERSAL_UP);
      }
    }
  }
}

void Component::transfer_focus_down_cycle() {
  if (is_focus_cycle_root()) {
    KeyboardFocusManager::get_current_focus_cycle_root(shared_from_this());
    if (auto to_focus = get_focus_traversal_policy()->get_default_component(shared_from_this())) {
      to_focus->request_focus(FocusEvent::Cause::TRAVERSAL_DOWN);
    }
  }
}

Screen* Component::get_screen() const {
  if (auto window = get_containing_window()) {
    return window->get_screen();
  }
  return nullptr;
}

EventQueue* Component::get_event_queue() const {
  return &get_screen()->get_event_queue();
}

std::shared_ptr<Window> Component::get_containing_window() const {
  for (auto component = const_cast<Component*>(this)->shared_from_this();; component = component->get_parent()) {
    if (auto window = std::dynamic_pointer_cast<Window>(component)) {
      return window;
    }
  }
  return {};
}

bool Component::process_key_bindings(KeyEvent &e) {
  auto const ks = [&e]() -> KeyStroke {
    if (e.id == KeyEvent::KEY_TYPED) {
      return {e.get_key_char()};
    } else {
      return {e.get_key_code(), e.modifiers};
    }
  }();

  if (process_key_binding(ks, e, WHEN_FOCUSED)) {
    return true;
  }

  auto parent = get_parent();
  for (; parent and not std::dynamic_pointer_cast<Window>(parent); parent = parent->get_parent()) {
    if (parent->process_key_binding(ks, e, WHEN_ANCESTOR_OF_FOCUSED_COMPONENT)) {
      return true;
    }
  }

  if (parent) {
    return process_key_bindings_for_all_components(e, parent);
  }
  return false;
}

bool Component::process_key_binding(const KeyStroke &ks, KeyEvent &e, InputCondition condition) {
  if (is_enabled()) {
    if (auto input_map = get_input_map(condition, false); input_map and this->action_map) {
      if (auto binding = input_map->at(ks); binding.has_value()) {
        if (auto action = this->action_map->at(binding.value())) {
          return notify_action(action, ks, e, shared_from_this());
        }
      }
    }
  }
  return false;
}

bool Component::notify_action(const std::shared_ptr<Action> &action, const KeyStroke &ks, KeyEvent &e, const std::shared_ptr<Component> &sender) {
  if (not action or not action->accept(sender)) {
    return false;
  }

  std::string command;
  // Get the command object.
  if (auto any = action->get_value(Action::ACTION_COMMAND_KEY)) {
    command = std::any_cast<decltype(command)>(*any);
  } else if (e.get_key_char() != KeyEvent::CHAR_UNDEFINED) {
    command = e.get_key_char();
  } else {
    return false;
  }
  auto event = ActionEvent { sender, command, e.modifiers, e.when };
  action->action_performed(event);
  return true;
}

bool Component::process_key_bindings_for_all_components(KeyEvent &e, std::shared_ptr<Component> container) {
  while (true) {
    if (KeyboardManager::fire_keyboard_action(e, container)) {
      return true;
    }

    if (auto popup_window = std::dynamic_pointer_cast<Popup::PopupWindow>(container)) {
      container = popup_window->get_owner();
    } else {
      return false;
    }
  }
}

bool Component::is_window(const std::shared_ptr<Component> &component) {
  return dynamic_cast<Window*>(component.get()) != nullptr;
}

void Component::add_impl(const std::shared_ptr<Component> &c, const std::any &constraints) noexcept (false) {
  if (is_window(c)) {
    throw std::runtime_error("Adding a window to a container");
  }

  for (auto parent = shared_from_this(); parent; parent = parent->get_parent()) {
    if (parent == c) {
      throw std::runtime_error("Adding component's parent to itself");
    }
  }

  // Reparent the component
  if (auto parent = c->get_parent()) {
    parent->remove(c);
  }

  // Add the specified component to the list of components
  // in this container
  this->components.emplace_back(c);

  // Set this container as the parent of the component
  c->set_parent(shared_from_this());

  invalidate();

  if (is_displayable()) {
    c->add_notify();
  }

  if (this->layout) {
    this->layout->add_layout_component(c, constraints);
  }
}

void Component::add_notify() {
  for (auto &&component : this->components) {
    component->add_notify();
  }
}

void Component::remove_impl(const std::shared_ptr<Component> &component) {
  if (is_displayable()) {
    component->remove_notify();
  }

  if (this->layout) {
    this->layout->remove_layout_component(component);
  }

  this->components.erase( //
      std::remove(this->components.begin(), this->components.end(), component), //
      this->components.end());

  component->set_parent(nullptr);
}

void Component::remove_notify() {
}

void Component::dispatch_event(Event &e) {
  log_event_ln(e);

  if (not e.focus_manager_is_dispatching) {
    // Now, with the event properly targeted to a lightweight
    // descendant if necessary, invoke the public focus retargeting
    // and dispatching function
    if (KeyboardFocusManager::dispatch_event(e)) {
      return;
    }
  }

  // TODO
//  if (auto focus_event = std::get_if<FocusEvent>(e.get()) and focusLog.isLoggable(PlatformLogger.Level.FINEST)) {
//    focusLog.finest("" + e);
//  }

  // MouseWheel may need to be retargeted here so that
  // AWTEventListener sees the event go to the correct
  // Component.  If the MouseWheelEvent needs to go to an ancestor,
  // the event is dispatched to the ancestor, and dispatching here
  // stops.
  if (auto mouse_wheel_event = dynamic_cast<MouseWheelEvent*>(&e)) {
    if (dispatch_mouse_wheel_to_ancestor(*mouse_wheel_event)) {
      return;
    }
  }

  /*
   * 3. If no one has consumed a key event, allow the
   *    KeyboardFocusManager to process it.
   */
  if (auto key_event = dynamic_cast<KeyEvent*>(&e)) {
    if (not key_event->consumed) {
      KeyboardFocusManager::process_key_event(shared_from_this(), *key_event);
      if (key_event->consumed) {
        return;
      }
    }
  }

  if (is_event_enabled(e)) {
    base::process_event(e);
  }
}

bool Component::dispatch_mouse_wheel_to_ancestor(MouseWheelEvent &e) {
  auto x = e.x + get_x();
  auto y = e.y + get_y();

  std::unique_lock lock(tree_mutex);

  auto ancestor = get_parent();
  while (ancestor and not ancestor->is_event_enabled(EventType::MOUSE_WHEEL)) {
    x += ancestor->get_x();
    y += ancestor->get_y();

    if (is_window(ancestor)) {
      break;
    }
    ancestor = ancestor->get_parent();
  }

  if (ancestor and ancestor->is_event_enabled(EventType::MOUSE_WHEEL)) {
    auto new_event = make_event<MouseWheelEvent>(ancestor, e.modifiers, x, y, e.wheel_rotation, e.when);
    ancestor->dispatch_event(new_event);
    if (new_event.consumed) {
      e.consumed = true;
    }
    return true;
  }
  return false;
}

std::shared_ptr<const std::unordered_set<KeyStroke>> Component::get_focus_traversal_keys(KeyboardFocusManager::FocusTraversalKeys id) const {
  if (this->focus_traversal_keys.empty()) {
    return {};
  }

  auto keyStrokes = this->focus_traversal_keys[id];

  if (keyStrokes) {
    return keyStrokes;
  } else if (auto parent = get_parent()) {
    return parent->get_focus_traversal_keys(id);
  } else {
    return KeyboardFocusManager::get_default_focus_traversal_keys(id);
  }
}

void Component::set_focus_traversal_keys(KeyboardFocusManager::FocusTraversalKeys id, const std::shared_ptr<const std::unordered_set<KeyStroke>> &keyStrokes) {
  this->focus_traversal_keys.resize(std::max(this->focus_traversal_keys.size(), size_t(id)));
  if (keyStrokes) {
    for (auto &&keyStroke : *keyStrokes) {
      for (auto &&keys : this->focus_traversal_keys) {
        if (keys and keys->contains(keyStroke)) {
          throw std::runtime_error("focus traversal keys must be unique for a Component");
        }
      }
    }
  }
}

void Component::event_listener_mask_updated(const EventTypeMask &removed, const EventTypeMask &added) {
  if (auto window = get_containing_window()) {
    window->enable_events_for_dispatching(added);
  }
}

void Component::show() {
  if (not this->visible) {
    std::unique_lock lock(tree_mutex);
    this->visible = true;
    create_hierarchy_events(HierarchyEvent::SHOWING_CHANGED, shared_from_this(), get_parent());
    repaint();

    post_event<ComponentEvent>(ComponentEvent::COMPONENT_SHOWN);

    if (auto parent = get_parent()) {
      parent->invalidate();
    }
  }
}

void Component::hide() {
  if (this->visible) {
    clear_current_focus_cycle_root_on_hide();
    clear_most_recent_focus_owner_on_hide();

    std::unique_lock lock(tree_mutex);
    this->visible = false;
    if (contains_focus() /*TODO and KeyboardFocusManager::is_auto_focus_transfer_enabled()*/) {
      transfer_focus(true);
    }
    create_hierarchy_events(HierarchyEvent::SHOWING_CHANGED, shared_from_this(), get_parent());
    repaint();

    post_event<ComponentEvent>(ComponentEvent::COMPONENT_HIDDEN);

    if (auto parent = get_parent()) {
      parent->invalidate();
    }
  }
}

bool Component::contains_focus() const {
  return is_parent_of(KeyboardFocusManager::get_focus_owner());
}

bool Component::is_parent_of(std::shared_ptr<Component> component) const {
  std::unique_lock lock(tree_mutex);
  while (component and component.get() != this and not is_window(component)) {
    component = component->get_parent();
  }
  return (component.get() == this);
}

void Component::clear_current_focus_cycle_root_on_hide() {
  auto focus_cycle_root = KeyboardFocusManager::get_current_focus_cycle_root();
  if (this == focus_cycle_root.get() or is_parent_of(focus_cycle_root)) {
    KeyboardFocusManager::get_current_focus_cycle_root(nullptr);
  }
}

void Component::clear_most_recent_focus_owner_on_hide() {
  std::unique_lock lock(tree_mutex);
  if (auto window = get_containing_window()) {
    auto focus_owner = KeyboardFocusManager::get_most_recent_focus_owner(window);
    auto reset = ((focus_owner.get() == this) or is_parent_of(focus_owner));
    auto storedComp = window->get_temporary_lost_component();
    if (is_parent_of(storedComp) or storedComp.get() == this) {
      window->set_temporary_lost_component(nullptr);
    }

    if (reset) {
      KeyboardFocusManager::set_most_recent_focus_owner(window, nullptr);
    }
  }
}

void Component::create_hierarchy_events(HierarchyEvent::Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) {
  for (auto &&component : this->components) {
    component->create_hierarchy_events(type, changed, changed_parent);
  }
  post_event<HierarchyEvent>(type, changed, changed_parent);
}

void Component::create_hierarchy_bounds_events(HierarchyBoundsEvent::Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) {
  for (auto &&component : this->components) {
    component->create_hierarchy_bounds_events(type, changed, changed_parent);
  }
  post_event<HierarchyBoundsEvent>(type, changed, changed_parent);
}

std::shared_ptr<Component> Component::get_mouse_event_target(int x, int y, bool include_self) const {
  std::unique_lock lock(this->tree_mutex);
  for (auto &&child : this->components) {
    if (child->visible and child->contains(x - child->get_x(), y - child->get_y())) {
      auto grand_child = child->get_mouse_event_target(x - child->get_x(), y - child->get_y(), true);
    }
  }
  if (contains(x, y)) {
    return const_cast<Component*>(this)->shared_from_this();
  }
  return nullptr;
}

/**
 * Convert a point from a screen coordinates to a component's coordinate system
 */
Point convert_point_from_screen(int x, int y, std::shared_ptr<Component> to) {
  do {
    x -= to->get_x();
    y -= to->get_y();
    to = to->get_parent();
  } while (to);
  return {x, y};
}

/**
 * Convert a point from a component's coordinate system to screen coordinates.
 */
Point convert_point_to_screen(int x, int y, std::shared_ptr<Component> from) {
  while (from) {
    x += from->get_x();
    y += from->get_y();
    from = from->get_parent();
  }
  return {x, y};
}

}
