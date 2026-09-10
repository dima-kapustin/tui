#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui::laf {

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
      break;
    }
    map = parent;
  }

  // A window-wide map's strokes enter the KeyboardManager's registry, so the
  // whole window offers them to this component (Swing registers them with the
  // component's own input map). The window is resolved when a key is fired,
  // not here: an accelerator may be installed while its component is not in a
  // window yet.
  if (condition == Component::WHEN_IN_FOCUSED_WINDOW) {
    c->register_with_keyboard_manager(false);
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

}
