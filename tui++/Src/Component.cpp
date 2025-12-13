#include <tui++/Popup.h>
#include <tui++/Border.h>
#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/Component.h>
#include <tui++/KeyStroke.h>
#include <tui++/KeyboardManager.h>
#include <tui++/DefaultFocusTraversalPolicy.h>

#include <tui++/Screen.h>

#include <tui++/util/log.h>

namespace tui {

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

bool Component::is_window(const std::shared_ptr<const Component> &component) {
  return dynamic_cast<const Window*>(component.get()) != nullptr;
}

void Component::assert_adding_none_window(const std::shared_ptr<const Component> &c) noexcept (false) {
  if (is_window(c)) {
    throw std::runtime_error("Adding a window to a container");
  }
}

void Component::assert_adding_none_cyclic(const std::shared_ptr<const Component> &c) noexcept (false) {
  for (auto parent = shared_from_this(); parent; parent = parent->get_parent()) {
    if (parent == c) {
      throw std::runtime_error("Adding component's parent to itself");
    }
  }
}

void Component::add_impl(const std::shared_ptr<Component> &c, const std::any &constraints, int z_order) noexcept (false) {
  if (z_order > (int) this->components.size() or (z_order < 0 and z_order != -1)) {
    throw std::runtime_error("Illegal component position");
  }

  assert_adding_none_window(c);
  assert_adding_none_cyclic(c);

  // Reparent the component
  if (auto parent = c->get_parent()) {
    parent->remove(c);
    if (z_order > (int) this->components.size()) {
      throw std::runtime_error("Illegal component position");
    }
  }

  if (z_order == -1) {
    this->components.emplace_back(c);
  } else {
    this->components.emplace(std::next(this->components.begin(), z_order), c);
  }

  c->set_parent(shared_from_this());

  invalidate();

  if (is_displayable()) {
    c->add_notify();
  }

  if (this->layout) {
    this->layout->add_layout_component(c, constraints);
  }

  if (is_event_enabled(EventType::CONTAINER)) {
    dispatch_event<ContainerEvent>(ContainerEvent::COMPONENT_ADDED, c);
  }

  c->create_hierarchy_events(HierarchyEvent::PARENT_CHANGED, c, shared_from_this());
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

  if (not e.is_being_dispatched_by_focus_manager) {
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
  auto accept = [](const Component *c) -> bool {
    return (c->event_mask & MOUSE_EVENT_MASK) or (c->get_event_listener_mask() & MOUSE_EVENT_MASK);
  };

  std::unique_lock lock(this->tree_mutex);
  for (auto &&child : this->components) {
    if (child->visible and child->contains(x - child->get_x(), y - child->get_y())) {
      if (auto descendant = child->get_mouse_event_target(x - child->get_x(), y - child->get_y(), true)) {
        if (accept(descendant.get())) {
          return descendant;
        }
      }
    }
  }
  if (include_self and accept(this) and contains(x, y)) {
    return const_cast<Component*>(this)->shared_from_this();
  }
  return nullptr;
}

int Component::get_component_z_order(const std::shared_ptr<const Component> &c) const {
  if (c) {
    std::unique_lock lock(this->tree_mutex);
    if (c->get_parent().get() == this) {
      return get_component_index(c);
    }
  }
  return -1;
}

void Component::set_component_z_order(const std::shared_ptr<Component> &c, int new_z_order) {
  std::unique_lock lock(this->tree_mutex);
  int old_z_order = get_component_z_order(c);
  // Store parent because remove will clear it
  auto parent = c->get_parent();
  if (parent.get() == this and new_z_order == old_z_order) {
    return;
  }

  if (new_z_order > (int) this->components.size() or new_z_order < 0) {
    throw std::runtime_error("Illegal component position");
  }

  if (parent.get() == this) {
    if (new_z_order >= (int) this->components.size()) {
      throw std::runtime_error(std::format("Illegal component position {} should be less than {}", new_z_order, this->components.size()));
    }
  }

  assert_adding_none_cyclic(c);
  assert_adding_none_window(c);

//  if (get_containing_window() != component->get_containing_window()) {
//    throw std::runtime_error("Component and container should be in the same top-level window");
//  }

  parent->remove_delicately(c, shared_from_this(), new_z_order);
  add_delicately(c, parent, new_z_order);
}

void Component::add_delicately(const std::shared_ptr<Component> &c, const std::shared_ptr<Component> &parent, int z_order) {
  auto was_displayable = c->is_displayable(); // is_displayable() may change when parent changes

  if (parent.get() != this) {
    if (z_order == -1) {
      this->components.emplace_back(c);
    } else {
      this->components.emplace(std::next(this->components.begin(), z_order), c);
    }
    c->parent = shared_from_this();
  } else if (z_order < (int) this->components.size()) {
    this->components[z_order] = c;
  }

  invalidate_if_valid();

  if (is_displayable() and not was_displayable) {
    c->add_notify();
  }

  if (parent.get() != this) {
    if (this->layout) {
      layout->add_layout_component(c);
    }

    if (is_event_enabled(EventType::CONTAINER)) {
      dispatch_event<ContainerEvent>(ContainerEvent::COMPONENT_ADDED, c);
    }

    c->create_hierarchy_events(HierarchyEvent::PARENT_CHANGED, c, shared_from_this());

    // If component is focus owner or parent container of focus owner check that after reparenting
    // focus owner moved out if new container prohibit this kind of focus owner.
    if (c->is_focus_owner() and not c->can_be_focus_owner_recursively()) {
      c->transfer_focus();
    } else if (auto focus_owner = KeyboardFocusManager::get_focus_owner()) {
      if (focus_owner and is_parent_of(focus_owner) and not focus_owner->can_be_focus_owner_recursively()) {
        focus_owner->transfer_focus();
      }
    }
  } else {
    c->create_hierarchy_events(HierarchyEvent::PARENT_CHANGED, c, shared_from_this());
  }
}

bool Component::remove_delicately(const std::shared_ptr<Component> &c, const std::shared_ptr<Component> &new_parent, int new_z_order) {
  auto old_z_order = get_component_z_order(c);
  auto need_remove_notify = c->is_displayable() and not new_parent->is_displayable();
  if (need_remove_notify) {
    c->remove_notify();
  }

  if (new_parent.get() != this) {
    if (this->layout) {
      this->layout->remove_layout_component(c);
    }
    c->parent.reset();

    this->components.erase(std::next(this->components.begin(), old_z_order));

    invalidate_if_valid();
  } else {
    // We should remove component and then
    // add it at the new_z_order without new_z_order decrement if even we shift components to the left
    // after remove. Consult the rules below:
    // 2->4: 012345 -> 013425, 2->5: 012345 -> 013452
    // 4->2: 012345 -> 014235
    this->components.erase(std::next(this->components.begin(), old_z_order));
    this->components.emplace(std::next(this->components.begin(), new_z_order), c);
  }

  if (c->parent.expired()) { // was actually removed
    if (is_event_enabled(EventType::CONTAINER)) {
      dispatch_event<ContainerEvent>(ContainerEvent::COMPONENT_REMOVED, c);
    }

    c->create_hierarchy_events(HierarchyEvent::PARENT_CHANGED, c, shared_from_this());
  }

  return need_remove_notify;
}

bool Component::can_contain_focus_owner(const std::shared_ptr<Component> &candidate) {
  if (not (is_enabled() and is_displayable() and is_visible() and is_focusable())) {
    return false;
  }

  if (is_focus_cycle_root()) {
    if (auto policy = std::dynamic_pointer_cast<DefaultFocusTraversalPolicy>(get_focus_traversal_policy()); policy and not policy->accept(candidate)) {
      return false;
    }
  }

  std::unique_lock lock(this->tree_mutex);
  if (auto parent = get_parent()) {
    return parent->can_contain_focus_owner(candidate);
  }
  return true;

}

bool Component::can_be_focus_owner_recursively() {
  if (not can_be_focus_owner()) {
    return false;
  }

  std::unique_lock lock(this->tree_mutex);
  if (auto parent = get_parent()) {
    return parent->can_contain_focus_owner(shared_from_this());
  }

  return true;
}

}
