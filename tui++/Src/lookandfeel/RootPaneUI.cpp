#include <tui++/lookandfeel/RootPaneUI.h>
#include <tui++/lookandfeel/LazyActionMap.h>

#include <tui++/Button.h>
#include <tui++/InputMap.h>
#include <tui++/KeyStroke.h>
#include <tui++/RootPane.h>
#include <tui++/event/KeyEvent.h>

#include <chrono>

namespace tui::laf {

// The theme keys of the root pane's keyboard resources. One pair of maps
// serves every root pane: the resources are the look-and-feel's, and what they
// act on is resolved per event (the source root pane's default button).
constexpr std::string_view ACTION_MAP_KEY = "RootPane.ActionMap";
constexpr std::string_view ANCESTOR_INPUT_MAP_KEY = "RootPane.AncestorInputMap";

// The command of the default button's click, the way the menu items name
// theirs.
constexpr std::string CLICK = "do_click";

void RootPaneUI::install_ui(std::shared_ptr<Component> const &c) {
  this->root_pane = static_cast<RootPane*>(c.get());

  install_keyboard_actions();
}

void RootPaneUI::uninstall_ui(std::shared_ptr<Component> const &c) {
  this->root_pane = nullptr;
}

void RootPaneUI::install_keyboard_actions() {
  auto action_map = LookAndFeel::get<std::shared_ptr<ActionMap>>(ACTION_MAP_KEY);
  if (not action_map) {
    action_map = std::make_shared<LazyActionMap>(load_action_map);
    LookAndFeel::put(ACTION_MAP_KEY, action_map);
  }
  LookAndFeel::replace_action_map(this->root_pane, action_map);

  auto input_map = LookAndFeel::get<std::shared_ptr<InputMap>>(ANCESTOR_INPUT_MAP_KEY);
  if (not input_map) {
    input_map = LookAndFeel::make_theme_resource<InputMap>();
    input_map->emplace(KeyStroke(KeyEvent::VK_ENTER, InputEvent::NO_MODIFIERS), CLICK);
    LookAndFeel::put(ANCESTOR_INPUT_MAP_KEY, input_map);
  }
  // Swing binds this stroke with the root pane's WHEN_IN_FOCUSED_WINDOW map.
  // The window-wide registry that serves that condition is not populated in
  // this port, so the ancestor map carries it instead: the same set of
  // components answers to it (everything below this root pane that does not
  // claim Enter itself) and the stroke still cannot reach another window.
  LookAndFeel::replace_input_map(this->root_pane, Component::WHEN_ANCESTOR_OF_FOCUSED_COMPONENT, input_map);
}

void RootPaneUI::load_action_map(LazyActionMap &map) {
  // Swing's JRootPane.DefaultAction: Enter clicks the window's default button
  // when it has one and it is enabled.
  map.emplace(CLICK, [](ActionEvent &e) {
    auto root_pane = std::static_pointer_cast<RootPane>(e.source);
    if (auto button = root_pane->get_default_button(); button and button->is_enabled()) {
      button->do_click(std::chrono::milliseconds::zero());
    }
  });
}

}
