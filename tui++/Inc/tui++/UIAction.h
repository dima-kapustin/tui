#pragma once

#include <tui++/Action.h>

namespace tui {
class UIAction: public Action {
public:
  UIAction(std::string const &name) {
    set_name(name);
  }

public:
  virtual bool accept(const std::shared_ptr<Object> &sender) const override {
    return true;
  }

  virtual bool is_enabled() const override {
    return accept(nullptr);
  }

  virtual void set_enabled(bool) override {
  }

protected:
  virtual PropertyBase* add_runtime_property(std::unique_ptr<PropertyBase> &&property) override {
    // This object is immutable
    return nullptr;
  }
};
}
