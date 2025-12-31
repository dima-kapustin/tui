#pragma once

#include <memory>
#include <ranges>
#include <string>
#include <algorithm>
#include <functional>
#include <unordered_map>

#include <tui++/Event.h>
#include <tui++/AbstractAction.h>

namespace tui {

class ActionMap {
  std::shared_ptr<ActionMap> parent;
  std::unordered_map<ActionKey, std::shared_ptr<Action>> map;

private:
  class ActionListenerAdapter: public AbstractAction {
    ActionListener action_listener;
  public:
    ActionListenerAdapter(const ActionListener &action_listener) :
        action_listener(action_listener) {
      set_enabled(true);
    }

  public:
    virtual void action_performed(ActionEvent &e) override {
      this->action_listener(e);
    }
  };

public:
  virtual ~ActionMap() {
  }

public:
  virtual std::vector<ActionKey> get_keys() const {
    auto keys = std::vector<ActionKey> { };
    keys.reserve(this->map.size());
    std::ranges::copy(this->map | std::views::keys, std::back_inserter(keys));
    return keys;
  }

  virtual std::vector<ActionKey> get_all_keys() const {
    auto keys = std::vector<ActionKey> { };
    keys.reserve(count_all_keys());
    collect_all_keys(keys);
    return keys;
  }

  virtual std::shared_ptr<Action> at(const ActionKey &key) const {
    if (auto pos = this->map.find(key); pos != this->map.end()) {
      return pos->second;
    } else if (this->parent) {
      return this->parent->at(key);
    }
    return {};
  }

  std::shared_ptr<ActionMap> get_parent() const {
    return this->parent;
  }

  virtual void set_parent(std::shared_ptr<ActionMap> const &parent) {
    this->parent = parent;
  }

  virtual void emplace(const ActionKey &key, const ActionListener &action_listener) {
    emplace(key, std::make_shared<ActionListenerAdapter>(action_listener));
  }

  virtual void emplace(const ActionKey &key, const std::shared_ptr<Action> &action) {
    this->map.emplace(key, action);
  }

  virtual void erase(const ActionKey &key) {
    this->map.erase(key);
  }

  virtual void clear() {
    this->map.clear();
  }

private:
  size_t count_all_keys() const {
    auto key_count = this->map.size();
    for (auto parent = this->parent; parent; parent = parent->parent) {
      key_count += parent->map.size();
    }
    return key_count;
  }

  void collect_all_keys(std::vector<ActionKey> &keys) const {
    if (this->parent) {
      this->parent->collect_all_keys(keys);
    }
    std::ranges::copy(this->map | std::views::keys, std::back_inserter(keys));
  }
};

}
