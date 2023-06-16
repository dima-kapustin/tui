#include <tui++/Border.h>
#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/Component.h>
#include <tui++/KeyStroke.h>
#include <tui++/KeyboardManager.h>

#include <tui++/Screen.h>

namespace tui {

std::recursive_mutex Component::tree_mutex;

std::shared_ptr<Window> Component::get_window_ancestor() const {
  if (auto window = std::dynamic_pointer_cast<Window>(const_cast<Component*>(this)->shared_from_this())) {
    return window;
  }

  for (auto ancestor = get_parent(); ancestor; ancestor = ancestor->get_parent()) {
    if (auto window = std::dynamic_pointer_cast<Window>(ancestor)) {
      return window;
    }
  }
  throw std::runtime_error("No window ancestor");
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

void Component::request_focus() {
  if (auto focus_component = get_focus_component(); not focus_component or not has_children()) {
    auto temporary = false;

    // generate the FOCUS_GAINED only if the component does not already have the focus
    auto ancestor = get_window_ancestor();
    auto ancestor_current_focus = ancestor->get_focus_component();

    auto top_window = get_top_window();
    auto top_window_current_focus = top_window->get_focus_component();

    if (ancestor_current_focus != shared_from_this() or (top_window_current_focus == shared_from_this() and not this->was_focus_owner)) {
      this->was_focus_owner = true;

      auto &event_queue = get_event_queue();

      if (auto last_focus_event = event_queue.get_last_focus_event()) {
        ancestor_current_focus = last_focus_event->source;
        if (ancestor_current_focus and ancestor_current_focus->get_window_ancestor() and ancestor_current_focus->get_window_ancestor()->is_enabled() and ancestor_current_focus->get_window_ancestor() != ancestor) {
          temporary = true;
        } else {
          temporary = false;
        }

        event_queue.push(std::make_shared<Event>(std::in_place_type<FocusEvent>, ancestor_current_focus, FocusEvent::FOCUS_LOST, temporary, shared_from_this()));
      } else {
        ancestor_current_focus = nullptr;
      }

      event_queue.push(std::make_shared<Event>(std::in_place_type<FocusEvent>, shared_from_this(), FocusEvent::FOCUS_GAINED, temporary, ancestor_current_focus));

      if (auto parent = get_parent()) {
        parent->set_focus(shared_from_this());
      }

      repaint();
    }
  } else {
    focus_component->request_focus();
  }
}

bool Component::transfer_focus(bool clear_on_failure) {
//  if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//      focusLog.finer("clearOnFailure = " + clearOnFailure);
//  }
  auto to_focus = get_next_focus_candidate();
  auto result = false;
  if (to_focus and not to_focus->is_focus_owner() and to_focus != shared_from_this()) {
//      res = toFocus.request_focus_in_window(FocusEvent::Cause::TRAVERSAL_FORWARD);
  }
  if (clear_on_failure and not result) {
//      if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//          focusLog.finer("clear global focus owner");
//      }
    KeyboardFocusManager::clear_global_focus_owner();
  }
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
//          result = toFocus->request_focus_in_window(FocusEvent.Cause.TRAVERSAL_BACKWARD);
    }
  }
  if (not result and clear_on_failure) {
//      if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//          focusLog.finer("clear global focus owner");
//      }
    KeyboardFocusManager::clear_global_focus_owner();
  }
//  if (focusLog.isLoggable(PlatformLogger.Level.FINER)) {
//      focusLog.finer("returning result: " + res);
//  }
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

    KeyboardFocusManager::set_global_current_focus_cycle_root(fcr);
//      rootAncestor->request_focus(FocusEvent.Cause.TRAVERSAL_UP);
  } else {
    if (auto window = get_containing_window()) {
      if (auto to_focus = window->get_focus_traversal_policy()->get_default_component(window)) {
        KeyboardFocusManager::set_global_current_focus_cycle_root(window);
//              toFocus->request_focus(FocusEvent.Cause.TRAVERSAL_UP);
      }
    }
  }
}

Screen* Component::get_screen() const {
  return get_window_ancestor()->get_screen();
}

std::shared_ptr<Window> Component::get_top_window() const {
  return get_screen()->get_top_window();
}

EventQueue& Component::get_event_queue() const {
  return get_screen()->get_event_queue();
}

std::shared_ptr<Window> Component::get_containing_window() const {
  auto component = const_cast<Component*>(this)->shared_from_this();
  while (component) {
    if (auto window = std::dynamic_pointer_cast<Window>(component)) {
      return window;
    }
    component = component->get_parent();
  }
  return {};
}

bool Component::process_key_bindings(KeyEvent &e) {
  auto const ks = [&e]() -> KeyStroke {
    if (e.type == KeyEvent::KEY_TYPED) {
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

bool Component::process_key_bindings_for_all_components(KeyEvent &e, const std::shared_ptr<Component> &container) {
  while (true) {
    if (KeyboardManager::fire_keyboard_action(e, container)) {
      return true;
    }

//    TODO if (auto window = std::dynamic_pointer_cast<Popup.HeavyWeightWindow>(container)) {
//      container = window.get_owner();
//    } else {
    return false;
//    }
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
