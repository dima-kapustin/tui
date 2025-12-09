#pragma once

#include <memory>
#include <unordered_map>

#include <tui++/KeyStroke.h>

namespace tui {

class MenuBar;
class KeyEvent;
class Component;

class KeyboardManager {
  using KeyMap = std::unordered_map<KeyStroke, std::vector<std::shared_ptr<Component>>>;

  static std::unordered_map<std::shared_ptr<Component>, KeyMap> component_map;
  static std::unordered_map<std::shared_ptr<Component>, std::vector<std::shared_ptr<MenuBar>>> menu_bar_map;

  static std::shared_ptr<Component> get_top_ancestor(const std::shared_ptr<Component> &component);

protected:
  static void fire_binding(const std::shared_ptr<Component> &component, const KeyStroke &key_stroke, KeyEvent &e);

public:
  static void register_key_stroke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component);
  static void unregister_key_storke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component);

  static void register_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);
  static void unregister_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);

  static bool fire_keyboard_action(KeyEvent &e, const std::shared_ptr<Component> &top_ancestor);
};

}

