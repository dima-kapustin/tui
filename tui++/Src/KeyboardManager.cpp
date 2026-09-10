#include <tui++/KeyboardManager.h>

#include <tui++/Window.h>
#include <tui++/MenuBar.h>
#include <tui++/PopupMenu.h>
#include <tui++/PopupWindow.h>
#include <tui++/event/KeyEvent.h>

namespace tui {

bool KeyboardManager::fire_keyboard_action(KeyEvent &e, const std::shared_ptr<Component> &top_ancestor) {
  auto key_stroke = KeyStroke { e };
  if (auto key_map_pos = this->component_map.find(key_stroke); key_map_pos != this->component_map.end()) {
    // There is no well defined order for WHEN_IN_FOCUSED_WINDOW
    // bindings, but we give precedence to those bindings just
    // added. This is done so that JMenus WHEN_IN_FOCUSED_WINDOW
    // bindings are accessed before those of the JRootPane (they
    // both have a WHEN_IN_FOCUSED_WINDOW binding for enter).
    for (auto i = key_map_pos->second.size(); i-- > 0;) {
      auto c = key_map_pos->second[i].lock();
      if (not c) {
        continue;
      }
      // Only the bindings of the window this key belongs to answer. The
      // window is resolved here rather than when the binding was made: the
      // accelerator of a menu row is installed while the row may not be in a
      // window yet, and its popup is up only some of the time.
      if (get_top_ancestor(c) != top_ancestor) {
        continue;
      }
      // Unlike the component that carries the binding, the binding does not
      // have to be showing: a menu row answers to its accelerator while its
      // menu is closed.
      if (c->is_enabled()) {
        fire_binding(c, key_stroke, e);
        if (e.consumed) {
          return true;
        }
      }
    }
  }
  // If no one handled it, then give the menus a crack. The're handled differently.
  // The key is to let any MenuBars process the event
  if (auto menu_bar_pos = this->menu_bar_map.find(top_ancestor); menu_bar_pos != this->menu_bar_map.end()) {
    auto key_stroke = KeyStroke { e };
    for (auto &&menu_bar : menu_bar_pos->second) {
      if (menu_bar->is_showing() and menu_bar->is_enabled()) {
        fire_binding(menu_bar, key_stroke, e);
        if (e.consumed) {
          return true;
        }
      }
    }
  }
  return e.consumed;
}

void KeyboardManager::fire_binding(const std::shared_ptr<Component> &component, const KeyStroke &key_stroke, KeyEvent &e) {
  if (component->process_key_binding(key_stroke, e, Component::WHEN_IN_FOCUSED_WINDOW)) {
    e.consumed = true;
  }
}

std::shared_ptr<Component> KeyboardManager::get_top_ancestor(const std::shared_ptr<Component> &c) {
  // Walk up to the component's window. The chain of a menu row ends at its
  // popup menu, which is a window of its own while its menu is open and has no
  // parent at all while it is closed: from there the popup's invoker (the menu
  // that owns it) leads on to the window the keys are dispatched to, so an
  // accelerator is found whether or not its menu is open.
  for (auto component = c; component;) {
    auto top = component;
    for (auto parent = top->get_parent(); parent; parent = parent->get_parent()) {
      top = parent;
      if (auto window = std::dynamic_pointer_cast<Window>(parent)) {
        // A popup window's keys belong to the window that owns it: a key
        // posted to the popup reaches its owner through
        // Component::process_key_bindings_for_all_components.
        while (auto popup_window = std::dynamic_pointer_cast<PopupWindow>(window)) {
          window = popup_window->get_owner();
        }
        return window and window->is_focusable_window() ? std::static_pointer_cast<Component>(window) : nullptr;
      }
    }

    if (auto popup_menu = std::dynamic_pointer_cast<PopupMenu>(top)) {
      component = popup_menu->get_invoker();
    } else {
      break;
    }
  }
  return {};
}

void KeyboardManager::register_key_stroke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component) {
  auto &&components = this->component_map[key_stroke];
  // A binding is registered once, and a dead component's entry does not
  // linger in front of it.
  std::erase_if(components, [&component](std::weak_ptr<Component> const &registered) {
    auto locked = registered.lock();
    return not locked or locked == component;
  });
  components.emplace_back(component);
}

void KeyboardManager::unregister_key_stroke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component) {
  if (auto pos = this->component_map.find(key_stroke); pos != this->component_map.end()) {
    auto &&components = pos->second;
    std::erase_if(components, [&component](std::weak_ptr<Component> const &registered) {
      auto locked = registered.lock();
      return not locked or locked == component;
    });
    if (components.empty()) {
      this->component_map.erase(pos);
    }
  }
}

void KeyboardManager::register_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {
  if (auto &&top = get_top_ancestor(menu_bar)) {
    auto &&menu_bars = this->menu_bar_map[top];
    if (std::find(menu_bars.begin(), menu_bars.end(), menu_bar) == menu_bars.end()) {
      menu_bars.emplace_back(menu_bar);
    }
  }
}

void KeyboardManager::unregister_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {
  if (auto &&top = get_top_ancestor(menu_bar)) {
    auto &&menu_bars = this->menu_bar_map[top];
    if (auto &&pos = std::find(menu_bars.begin(), menu_bars.end(), menu_bar); pos != menu_bars.end()) {
      menu_bars.erase(pos);
    }
  }
}

}
