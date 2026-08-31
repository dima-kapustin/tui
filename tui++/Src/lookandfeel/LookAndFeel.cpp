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

}
