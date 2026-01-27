#pragma once

#include <ranges>
#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>

#include <tui++/Themable.h>
#include <tui++/ActionMap.h>
#include <tui++/KeyStroke.h>

namespace tui {

class InputMap: public Themable {
  std::shared_ptr<InputMap> parent;
  std::unordered_map<KeyStroke, ActionKey> map;

public:
  std::optional<ActionKey> at(const KeyStroke &key_stroke) const {
    auto pos = this->map.find(key_stroke);
    if (pos != this->map.end()) {
      return pos->second;
    } else if (this->parent) {
      return this->parent->at(key_stroke);
    }
    return std::nullopt;
  }

  void emplace(const KeyStroke &key_stroke, const ActionKey &action_key) {
    this->map.emplace(key_stroke, action_key);
  }

  void erase(const KeyStroke &key_stroke) {
    this->map.erase(key_stroke);
  }

  std::shared_ptr<InputMap> get_parent() const {
    return this->parent;
  }

  void set_parent(std::shared_ptr<InputMap> const &parent) {
    this->parent = parent;
  }

  void clear() {
    this->map.clear();
  }

  std::vector<KeyStroke> get_keys() const {
    auto keys = std::vector<KeyStroke> { };
    keys.reserve(this->map.size());
    std::ranges::copy(this->map | std::views::keys, std::back_inserter(keys));
    return keys;
  }

  std::vector<KeyStroke> get_all_keys() const {
    auto keys = std::vector<KeyStroke> { };
    keys.reserve(count_all_keys());
    collect_all_keys(keys);
    return keys;
  }

private:
  size_t count_all_keys() const {
    auto key_count = this->map.size();
    for (auto parent = this->parent; parent; parent = parent->parent) {
      key_count += parent->map.size();
    }
    return key_count;
  }

  void collect_all_keys(std::vector<KeyStroke> &keys) const {
    if (this->parent) {
      this->parent->collect_all_keys(keys);
    }
    std::ranges::copy(this->map | std::views::keys, std::back_inserter(keys));
  }
};

}

