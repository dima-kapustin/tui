#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

#include <tui++/Event.h>
#include <tui++/Action.h>

namespace tui {

class ActionMap {
  std::weak_ptr<ActionMap> parent;
  std::unordered_map<ActionKey, std::shared_ptr<Action>> map;

private:
  class FunctionalAdapter: public Action {
    ActionListener action_listener;
  public:
    FunctionalAdapter(const ActionListener &action_listener) :
        action_listener(action_listener) {
      set_enabled(true);
    }

  public:
    void action_performed(ActionEvent &e) override {
      this->action_listener(e);
    }
  };

public:
  std::shared_ptr<Action> at(const ActionKey &key) const {
    if (auto pos = this->map.find(key); pos != this->map.end()) {
      return pos->second;
    } else if (auto parent = get_parent()) {
      return parent->at(key);
    }
    return {};
  }

  std::shared_ptr<ActionMap> get_parent() const {
    return this->parent.lock();
  }

  void emplace(const ActionKey &key, const ActionListener &action_listener) {
    emplace(key, std::make_shared<FunctionalAdapter>(action_listener));
  }

  void emplace(const ActionKey &key, const std::shared_ptr<Action> &action) {
    this->map.emplace(key, action);
  }

  void erase(const ActionKey &key) {
    this->map.erase(key);
  }

  void clear() {
    this->map.clear();
  }
};

}
