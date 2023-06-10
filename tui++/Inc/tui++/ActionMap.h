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
  class FunctionAdapter: public Action {
    std::function<void(const ActionEvent&)> action;
  public:
    FunctionAdapter(const std::function<void(const ActionEvent&)> &action) :
        action(action) {
    }

  public:
    bool is_enabled() const override {
      return true;
    }

    void action_performed(const ActionEvent &event) override {
      this->action(event);
    }
  };

public:
  std::shared_ptr<Action> at(const ActionKey &key) const {
    auto pos = this->map.find(key);
    if (pos != this->map.end()) {
      return pos->second;
    } else if (auto parent = get_parent()) {
      return parent->at(key);
    }
    return {};
  }

  std::shared_ptr<ActionMap> get_parent() const {
    return this->parent.lock();
  }

  void emplace(const ActionKey &key, const std::function<void(const ActionEvent&)> &action) {
    emplace(key, std::make_shared<FunctionAdapter>(action));
  }

  void emplace(const ActionKey &key, const std::shared_ptr<Action> &action) {
    this->map.emplace(key, action);
  }

  void erase(const ActionKey &key) {
    this->map.erase(key);
  }
};

}
