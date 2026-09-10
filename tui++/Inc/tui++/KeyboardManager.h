#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <tui++/KeyStroke.h>

namespace tui {

class MenuBar;
class KeyEvent;
class Component;

class KeyboardManager {
  // The components that bind a stroke, most recently registered last. A menu
  // row's accelerator and a root pane's Enter may bind the same stroke; which
  // of them a key reaches is decided when it is fired, by the window the key
  // was dispatched to. The references are weak: a row that is dropped from
  // its menu without ever becoming displayable cannot unregister itself, and
  // its binding must not keep it alive.
  using StrokeBindings = std::vector<std::weak_ptr<Component>>;

  std::unordered_map<KeyStroke, StrokeBindings> component_map;
  std::unordered_map<std::shared_ptr<Component>, std::vector<std::shared_ptr<MenuBar>>> menu_bar_map;

  // The window a component's keys are dispatched to, or null while the
  // component is not in one.
  std::shared_ptr<Component> get_top_ancestor(const std::shared_ptr<Component> &component);

public:
  // The registry of WHEN_IN_FOCUSED_WINDOW strokes, shared by every window.
  // It is created here like the other managers (KeyboardFocusManager,
  // MenuSelectionManager, ...): a null one would be dereferenced by the first
  // component that processed a key binding through its window.
  static inline std::shared_ptr<KeyboardManager> single = std::make_shared<KeyboardManager>();

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

