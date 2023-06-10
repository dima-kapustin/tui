#pragma once

#include <ranges>
#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>

#include <tui++/ActionMap.h>
#include <tui++/KeyStroke.h>

namespace tui {

class InputMap {
  std::weak_ptr<InputMap> parent;
  std::unordered_map<KeyStroke, ActionKey> map;

private:
  size_t count_all_keys() const {
    auto key_count = this->map.size();
    for (auto parent = get_parent(); parent; parent = parent->get_parent()) {
      key_count += parent->map.size();
    }
    return key_count;
  }

  void get_all_keys(std::vector<KeyStroke> &keys) const {
    if (auto parent = get_parent()) {
      parent->get_all_keys(keys);
    }
    std::ranges::copy(this->map | std::views::keys, std::back_inserter(keys));
  }

public:
  virtual ~InputMap() {
  }

public:
  std::optional<ActionKey> at(const KeyStroke &key_stroke) const {
    auto pos = this->map.find(key_stroke);
    if (pos != this->map.end()) {
      return pos->second;
    } else if (auto parent = get_parent()) {
      return parent->at(key_stroke);
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
    return this->parent.lock();
  }

  void clear() {
    this->map.clear();
  }

  std::vector<KeyStroke> get_keys() const {
    std::vector<KeyStroke> keys;
    std::ranges::copy(this->map | std::views::keys, std::back_inserter(keys));
    return keys;
  }

  std::vector<KeyStroke> get_all_keys() const {
    std::vector<KeyStroke> keys;
    keys.reserve(count_all_keys());
    get_all_keys(keys);
    return keys;
  }
};

}

