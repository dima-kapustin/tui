#pragma once

#include <any>
#include <string>
#include <unordered_map>

#include <tui++/event/ActionEvent.h>
#include <tui++/event/EventListener.h>

namespace tui {

class Object;

class Action: public EventListener<ActionEvent> {
  std::unordered_map<std::string, std::any> map;
public:
  constexpr static std::string ACTION_COMMAND_KEY = "ActionCommand";

public:
  virtual ~Action() {
  }

  virtual bool accept(const std::shared_ptr<Object> &sender) const {
    return is_enabled();
  }

  virtual bool is_enabled() const = 0;

  const std::any* get_value(const std::string &key) const {
    if (auto pos = this->map.find(key); pos != this->map.end()) {
      return &pos->second;
    }
    return {};
  }

  void put_value(const std::string &key, const std::any &value) {
    this->map[key] = value;
  }
};

class BasicAction: public Action {

};

}
