#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

#include <tui++/Component.h>
#include <tui++/Theme.h>

#include <any>
#include <string_view>
#include <unordered_map>

namespace tui {
class Icon;
class Frame;
class Panel;
class Border;
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
  static std::shared_ptr<Theme> theme;

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

  static std::shared_ptr<Icon> get_icon(std::string_view const &key) {
    return get<std::shared_ptr<Icon>>(key);
  }

  static std::shared_ptr<Border> get_border(std::string_view const &key) {
    return get<std::shared_ptr<Border>>(key);
  }

  template<typename T>
  static void put(std::string_view const &key, T const &value) {
    properties.emplace(key, value);
  }

  static void put(std::initializer_list<std::pair<std::string_view, std::any>> &&values);

  template<typename T, typename ... Args>
  static std::shared_ptr<T> make_theme_resource(Args &&... args) {
    return theme->make_resource<T>(std::forward<Args>(args)...);
  }

  template<typename T>
  static void install(Component *c, const char *key, T &&value) {
    if (auto *property = c->get_property(key)) {
      if (not property->is_value_set()) {
        property->set_value(std::forward<T>(value), true);
      }
    }
  }

  static void install_border(Component *c, std::string_view const &key) {
    if (auto &&border = c->get_border(); not border or is_theme_resource(border)) {
      c->set_border(get_border(key));
    }
  }

  static void install_colors(Component *c, std::string_view const &background_color_key, std::string_view const &foreground_color_key) {
    if (auto &&background_color = c->get_background_color(); not background_color or is_theme_resource(background_color.value())) {
      c->set_background_color(get<std::optional<Color>>(background_color_key));
    }

    if (auto &&foreground_color = c->get_foreground_color(); not foreground_color or is_theme_resource(foreground_color.value())) {
      c->set_foreground_color(get<std::optional<Color>>(foreground_color_key));
    }
  }

  static std::shared_ptr<ActionMap> get_action_map(Component const *c);
  static std::shared_ptr<InputMap> get_input_map(Component const *c, Component::InputCondition condition);

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
