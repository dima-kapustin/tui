#pragma once

#include <tui++/lookandfeel/ComponentUI.h>

#include <tui++/Component.h>
#include <tui++/Theme.h>

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
  static std::shared_ptr<Theme> theme;
  static std::shared_ptr<LookAndFeel> current;

public:
  virtual ~LookAndFeel() = default;

  static std::shared_ptr<Theme> const& get_theme() {
    return theme;
  }

  static void set_theme(std::shared_ptr<Theme> const &theme);

  static void set_current(std::shared_ptr<LookAndFeel> const &lf) {
    current = lf;
  }

  template<typename T>
  static T get(std::string_view const &key, T &&default_value = { }) {
    return theme->get<T>(key, std::forward<T>(default_value));
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
    return theme->get_icon(key);
  }

  static std::shared_ptr<Border> get_border(std::string_view const &key) {
    return theme->get_border(key);
  }

  template<typename T>
  static void put(std::string_view const &key, T &&value) {
    theme->put(key, std::forward<T>(value));
  }

  template<typename T, typename ... Args>
  static std::shared_ptr<T> make_theme_resource(Args &&... args) {
    return theme->make_shared_resource<T>(std::forward<Args>(args)...);
  }

  template<typename T>
  constexpr std::enable_if_t<std::is_base_of_v<Themable, T>, T&&> make_theme_resource(T &&obj) {
    return theme->make_resource(std::move(obj));
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
  virtual std::shared_ptr<FrameUI> create_frame_ui(Frame *c) = 0;
  virtual std::shared_ptr<PanelUI> create_panel_ui(Panel *c) = 0;
  virtual std::shared_ptr<ButtonUI> create_button_ui(Button *c) = 0;
  virtual std::shared_ptr<MenuItemUI> create_menu_item_ui(MenuItem *c) = 0;
  virtual std::shared_ptr<MenuUI> create_menu_ui(Menu *c) = 0;
  virtual std::shared_ptr<MenuBarUI> create_menu_bar_ui(MenuBar *c) = 0;
  virtual std::shared_ptr<RootPaneUI> create_root_pane_ui(RootPane *c) = 0;
  virtual std::shared_ptr<PopupMenuUI> create_popup_menu_ui(PopupMenu *c) = 0;
  virtual std::shared_ptr<PopupMenuSeparatorUI> create_popup_menu_separator_ui(PopupMenuSeparator *c) = 0;
  virtual std::shared_ptr<SeparatorUI> create_separator_ui(Separator *c) = 0;
  virtual std::shared_ptr<ToggleButtonUI> create_toggle_button_ui(ToggleButton *c) = 0;

  static std::shared_ptr<FrameUI> create_ui(Frame *c) {
    return current->create_frame_ui(c);
  }
  static std::shared_ptr<PanelUI> create_ui(Panel *c) {
    return current->create_panel_ui(c);
  }
  static std::shared_ptr<ButtonUI> create_ui(Button *c) {
    return current->create_button_ui(c);
  }
  static std::shared_ptr<MenuItemUI> create_ui(MenuItem *c) {
    return current->create_menu_item_ui(c);
  }
  static std::shared_ptr<MenuUI> create_ui(Menu *c) {
    return current->create_menu_ui(c);
  }
  static std::shared_ptr<MenuBarUI> create_ui(MenuBar *c) {
    return current->create_menu_bar_ui(c);
  }
  static std::shared_ptr<RootPaneUI> create_ui(RootPane *c) {
    return current->create_root_pane_ui(c);
  }
  static std::shared_ptr<PopupMenuUI> create_ui(PopupMenu *c) {
    return current->create_popup_menu_ui(c);
  }
  static std::shared_ptr<PopupMenuSeparatorUI> create_ui(PopupMenuSeparator *c) {
    return current->create_popup_menu_separator_ui(c);
  }
  static std::shared_ptr<SeparatorUI> create_ui(Separator *c) {
    return current->create_separator_ui(c);
  }
  static std::shared_ptr<ToggleButtonUI> create_ui(ToggleButton *c) {
    return current->create_toggle_button_ui(c);
  }
};

}
