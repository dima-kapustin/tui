#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

#include <tui++/Component.h>

#include <any>
#include <unordered_map>

namespace tui {
class Frame;
class Panel;
class Button;
class Dialog;
class Menu;
class MenuBar;
class MenuItem;
class RootPane;
class PopupMenu;
class PopupMenuSeparator;
class Separator;
class ToggleButton;

class InputMap;
}

namespace tui::laf {

class FrameUI;
class PanelUI;
class ButtonUI;
class MenuUI;
class MenuBarUI;
class MenuItemUI;
class RootPaneUI;
class PopupMenuUI;
class PopupMenuSeparatorUI;
class SeparatorUI;
class ToggleButtonUI;

class LookAndFeel {
  static std::unordered_map<std::string_view, std::any> properties;

public:
  template<typename T>
  static T get(std::string_view const &key, T &&default_value = { }) {
    if (auto pos = properties.find(key); pos != properties.end()) {
      if (auto *value = std::any_cast<T>(&pos->second)) {
        return *value;
      }
    }
    return default_value;
  }

  template<typename T>
  static T get(Component const *c, std::string_view const &key, T &&default_value = { }) {
    if (auto *value = c->get_client_property<T>(key)) {
      return *value;
    } else {
      return get<T>(key, std::forward<T>(default_value));
    }
  }

  template<typename T>
  static void put(std::string_view const &key, T const &value) {
    properties.emplace(key, value);
  }

  static void put(std::initializer_list<std::pair<std::string_view, std::any>> &&values);

  static std::shared_ptr<ActionMap> get_action_map(Component *c);
  static std::shared_ptr<InputMap> get_input_map(Component *c, Component::InputCondition condition);

  static void replace_input_map(Component *c, Component::InputCondition condition, std::shared_ptr<InputMap> const &new_map);
  static void replace_action_map(Component *c, std::shared_ptr<ActionMap> const &new_map);

public:
  static std::shared_ptr<FrameUI> create_ui(Frame *c);
  static std::shared_ptr<PanelUI> create_ui(Panel *c);
  static std::shared_ptr<ButtonUI> create_ui(Button *c);
  static std::shared_ptr<MenuItemUI> create_ui(MenuItem *c);
  static std::shared_ptr<MenuUI> create_ui(Menu *c);
  static std::shared_ptr<MenuBarUI> create_ui(MenuBar *c);
  static std::shared_ptr<RootPaneUI> create_ui(RootPane *c);
  static std::shared_ptr<PopupMenuUI> create_ui(PopupMenu *c);
  static std::shared_ptr<PopupMenuSeparatorUI> create_ui(PopupMenuSeparator *c);
  static std::shared_ptr<SeparatorUI> create_ui(Separator *c);
  static std::shared_ptr<ToggleButtonUI> create_ui(ToggleButton *c);
};

}
