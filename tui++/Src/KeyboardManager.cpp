#include <tui++/KeyboardManager.h>

#include <tui++/Window.h>
#include <tui++/MenuBar.h>
#include <tui++/event/KeyEvent.h>

namespace tui {

bool KeyboardManager::fire_keyboard_action(KeyEvent &e, const std::shared_ptr<Component> &top_ancestor) {
  if (auto key_map_pos = component_map.find(top_ancestor); key_map_pos != component_map.end()) {
    auto key_stroke = KeyStroke { e };
    if (auto components_pos = key_map_pos->second.find(key_stroke); components_pos != key_map_pos->second.end()) {
      // There is no well defined order for WHEN_IN_FOCUSED_WINDOW
      // bindings, but we give precedence to those bindings just
      // added. This is done so that JMenus WHEN_IN_FOCUSED_WINDOW
      // bindings are accessed before those of the JRootPane (they
      // both have a WHEN_IN_FOCUSED_WINDOW binding for enter).
      for (auto i = components_pos->second.size() - 1;; --i) {
        auto &&c = components_pos->second[i];
        if (c->is_showing() and c->is_enabled()) {
          fire_binding(c, key_stroke, e);
          if (e.consumed) {
            return true;
          }
        }

        if (i == 0) {
          break;
        }
      }
    }
  }
  // If no one handled it, then give the menus a crack. The're handled differently.
  // The key is to let any MenuBars process the event
  if (auto menu_bar_pos = menu_bar_map.find(top_ancestor); menu_bar_pos != menu_bar_map.end()) {
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
  for (auto p = c->get_parent(); p; p = p->get_parent()) {
    if (auto window = std::dynamic_pointer_cast<Window>(p); window and window->is_focusable_window()) {
      return p;
    }
  }
  return {};
}

void KeyboardManager::register_key_stroke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component) {
  if (auto &&top = get_top_ancestor(component)) {
    auto &&components = component_map[top][key_stroke];
    if (std::find(components.begin(), components.end(), component) == components.end()) {
      components.emplace_back(component);
    }
  }
}

void KeyboardManager::unregister_key_storke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component) {
  if (auto &&top = get_top_ancestor(component)) {
    auto &&components = component_map[top][key_stroke];
    if (auto &&pos = std::find(components.begin(), components.end(), component); pos != components.end()) {
      components.erase(pos);
    }
  }
}

void KeyboardManager::register_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {
  if (auto &&top = get_top_ancestor(menu_bar)) {
    auto &&menu_bars = menu_bar_map[top];
    if (std::find(menu_bars.begin(), menu_bars.end(), menu_bar) == menu_bars.end()) {
      menu_bars.emplace_back(menu_bar);
    }
  }
}

void KeyboardManager::unregister_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {
  if (auto &&top = get_top_ancestor(menu_bar)) {
    auto &&menu_bars = menu_bar_map[top];
    if (auto &&pos = std::find(menu_bars.begin(), menu_bars.end(), menu_bar); pos != menu_bars.end()) {
      menu_bars.erase(pos);
    }
  }
}

}
