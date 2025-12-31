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

  std::unordered_map<std::shared_ptr<Component>, KeyMap> component_map;
  std::unordered_map<std::shared_ptr<Component>, std::vector<std::shared_ptr<MenuBar>>> menu_bar_map;

  std::shared_ptr<Component> get_top_ancestor(const std::shared_ptr<Component> &component);

public:
  static inline std::shared_ptr<KeyboardManager> single = std::shared_ptr<KeyboardManager>();

public:
  void register_key_stroke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component);
  void unregister_key_stroke(const KeyStroke &key_stroke, const std::shared_ptr<Component> &component);

  void register_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);
  void unregister_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);

  bool fire_keyboard_action(KeyEvent &e, const std::shared_ptr<Component> &top_ancestor);

protected:
  void fire_binding(const std::shared_ptr<Component> &component, const KeyStroke &key_stroke, KeyEvent &e);
};

}

