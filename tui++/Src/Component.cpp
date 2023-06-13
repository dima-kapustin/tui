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
        ancestor_current_focus = last_focus_event->focus.source;
        if (ancestor_current_focus and ancestor_current_focus->get_window_ancestor() and ancestor_current_focus->get_window_ancestor()->is_enabled() and ancestor_current_focus->get_window_ancestor() != ancestor) {
          temporary = true;
        } else {
          temporary = false;
        }

        event_queue.push(std::make_shared<Event>(ancestor_current_focus, FocusEvent::FOCUS_LOST, temporary, shared_from_this()));
      } else {
        ancestor_current_focus = nullptr;
      }

      event_queue.push(std::make_shared<Event>(shared_from_this(), FocusEvent::FOCUS_GAINED, temporary, ancestor_current_focus));

      if (auto parent = get_parent()) {
        parent->set_focus(shared_from_this());
      }

      repaint();
    }
  } else {
    focus_component->request_focus();
  }
}

void Component::transfer_focus() {
  if (not has_children()) {
    return;
  }
  /* Put a FOCUS_LOST event on the queue for the component that is losing the focus. If the current focus is a Container, then this
   * method will have been called by that container (which would already have posted a FOCUS_LOST event for its own contained
   * component that was losing focus). if ((focusOwner instanceof Container) == false) { FocusEvent evt = new
   * FocusEvent(AWTEvent.FOCUS_LOST, focusOwner); EventQueue evtQueue = Toolkit.getDefaultToolkit().getSystemEventQueue();
   * evtQueue.postEvent(evt); } */

  /* Determine which component should get focus next. */
  auto index = get_component_index(this->focus_component.lock().get());
  if (index == size_t(-1)) {
    throw std::runtime_error("focus component not found in parent");
  }

  std::shared_ptr<Component> focus_candidate;

  for (;;) {
    /* If the focus was owned by the last component in this container, the new focus should go to the next component in the parent
     * container, IF THERE IS A PARENT (this container may be a Window, in which case the parent is null). */
    if (++index >= this->components.size()) {
      std::shared_ptr<Component> parent;
      if (not is_focus_cycle_root() and (parent = get_parent())) {
        parent->transfer_focus();
        return;
      } else {
        /* Don't need to worry about infinite loops. Worst case, we should just end up where we started. */
        index = 0;
      }
    }

    focus_candidate = this->components[index];

    /* If the next component will not accept the focus, continue trying until we get one that does. */
    if (focus_candidate->is_focusable()) {
      break;
    }
  }

  if (this->focus_component.lock() != focus_candidate) {
    focus_candidate->first_focus();
    focus_candidate->request_focus();
  }
}

void Component::transfer_focus_backward() {
  if (not has_children()) {
    return;
  }

  /* Put a FOCUS_LOST event on the queue for the component that is losing the focus. If the current focus is a Container, then this
   * method will have been called by that container (which would already have posted a FOCUS_LOST event for its own contained
   * component that was losing focus). if ((focusOwner instanceof Container) == false) { FocusEvent evt = new
   * FocusEvent(AWTEvent.FOCUS_LOST, focusOwner); EventQueue evtQueue = Toolkit.getDefaultToolkit().getSystemEventQueue();
   * evtQueue.postEvent(evt); } */

  /* Determine which component should get focus next. */
  auto index = get_component_index(this->focus_component.lock().get());
  if (index == size_t(-1)) {
    throw std::runtime_error("focus component not found in parent");
  }

  std::shared_ptr<Component> focus_candidate;

  for (;;) {
    /* If the focus was owned by the first component in this container, the new focus should go to the previous component in the
     * parent container, IF THERE IS A PARENT (this container may be a Window, in which case the parent is null). */
    if (index == 0) {
      std::shared_ptr<Component> parent;
      if (not is_focus_cycle_root() and (parent = get_parent())) {
        parent->transfer_focus_backward();
        return;
      } else {
        index = this->components.size() - 1;
      }
    } else {
      index -= 1;
    }

    focus_candidate = this->components[index];

    /* If the next component will not accept the focus, continue trying until we get one that does. */
    if (focus_candidate->is_focusable()) {
      break;
    }
  }
  if (this->focus_component.lock() != focus_candidate) {
    focus_candidate->last_focus();
    focus_candidate->request_focus();
  }
}

void Component::transfer_focus_up_cycle() {

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
//        return SwingUtilities.notifyAction(action, ks, e, this, e.getModifiers());
        }
      }
    }
  }
  return false;
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
