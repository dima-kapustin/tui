#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/lookandfeel/PanelUI.h>
#include <tui++/lookandfeel/FrameUI.h>
#include <tui++/lookandfeel/ButtonUI.h>
#include <tui++/lookandfeel/RootPaneUI.h>
#include <tui++/lookandfeel/ToggleButtonUI.h>

#include <tui++/lookandfeel/MenuUI.h>
#include <tui++/lookandfeel/MenuBarUI.h>
#include <tui++/lookandfeel/MenuItemUI.h>
#include <tui++/lookandfeel/PopupMenuUI.h>
#include <tui++/lookandfeel/SeparatorUI.h>
#include <tui++/lookandfeel/PopupMenuSeparatorUI.h>

#include <tui++/lookandfeel/SystemColorKeys.h>

namespace tui::laf {

class TuiTheme: public Theme {
public:
  TuiTheme() {
    LookAndFeel::put( //
        { { "Menu.SubmenuPopupOffsetX", 0 }, //
          { "Menu.SubmenuPopupOffsetY", 0 }, //
          { "Menu.MenuPopupOffsetX", 0 }, //
          { "Menu.MenuPopupOffsetY", 0 }, //
        });

    install_system_colors();
  }

  void install_system_colors() {
    update_system_colors();

    using namespace SystemColorKeys;
    for (auto &&key : SYSTEM_COLOR_KEYS) {
      LookAndFeel::put(key, get_system_color(key));
    }
  }
};

std::unordered_map<std::string_view, std::any> LookAndFeel::properties;
std::shared_ptr<Theme> LookAndFeel::theme = std::make_shared<TuiTheme>();

void LookAndFeel::put(std::initializer_list<std::pair<std::string_view, std::any>> &&values) {
  for (auto&& [key, value] : values) {
    properties.emplace(std::move(key), std::move(value));
  }
}

std::shared_ptr<ActionMap> LookAndFeel::get_action_map(Component const *c) {
  for (auto map = c->get_action_map(false); map;) {
    auto parent = map->get_parent();
    if (is_theme_resource(parent)) {
      return parent;
    }
    map = parent;
  }
  return {};
}

std::shared_ptr<InputMap> LookAndFeel::get_input_map(Component const *c, Component::InputCondition condition) {
  for (auto map = c->get_input_map(condition, false); map;) {
    auto parent = map->get_parent();
    if (is_theme_resource(parent)) {
      return parent;
    }
    map = parent;
  }
  return {};
}

void LookAndFeel::replace_input_map(Component *c, Component::InputCondition condition, std::shared_ptr<InputMap> const &new_map) {
  for (auto map = c->get_input_map(condition, new_map != nullptr); map;) {
    auto parent = map->get_parent();
    if (not parent or is_theme_resource(parent)) {
      map->set_parent(new_map);
      return;
    }
    map = parent;
  }
}

void LookAndFeel::replace_action_map(Component *c, std::shared_ptr<ActionMap> const &new_map) {
  for (auto map = c->get_action_map(new_map != nullptr); map;) {
    auto parent = map->get_parent();
    if (not parent or is_theme_resource(parent)) {
      map->set_parent(new_map);
      return;
    }
    map = parent;
  }
}

std::shared_ptr<FrameUI> LookAndFeel::create_ui(Frame *c) {
  return std::make_shared<FrameUI>();
}

std::shared_ptr<PanelUI> LookAndFeel::create_ui(Panel *c) {
  return std::make_shared<PanelUI>();
}

std::shared_ptr<ButtonUI> LookAndFeel::create_ui(Button *c) {
  return std::make_shared<ButtonUI>();
}

std::shared_ptr<MenuUI> LookAndFeel::create_ui(Menu *c) {
  return std::make_shared<MenuUI>();
}

std::shared_ptr<MenuBarUI> LookAndFeel::create_ui(MenuBar *c) {
  return std::make_shared<MenuBarUI>();
}

std::shared_ptr<MenuItemUI> LookAndFeel::create_ui(MenuItem *c) {
  return std::make_shared<MenuItemUI>();
}

std::shared_ptr<RootPaneUI> LookAndFeel::create_ui(RootPane *c) {
  return std::make_shared<RootPaneUI>();
}

std::shared_ptr<PopupMenuUI> LookAndFeel::create_ui(PopupMenu *c) {
  return std::make_shared<PopupMenuUI>();
}

std::shared_ptr<PopupMenuSeparatorUI> LookAndFeel::create_ui(PopupMenuSeparator *c) {
  return std::make_shared<PopupMenuSeparatorUI>();
}

std::shared_ptr<SeparatorUI> LookAndFeel::create_ui(Separator *c) {
  return std::make_shared<SeparatorUI>();
}

std::shared_ptr<ToggleButtonUI> LookAndFeel::create_ui(ToggleButton *c) {
  return std::make_shared<ToggleButtonUI>();
}

}
