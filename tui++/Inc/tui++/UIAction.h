#pragma once

#include <tui++/Action.h>

#include <string>

namespace tui {
// A look-and-feel action: it holds its name, and nothing else -- the values of
// a program action live in the properties of an AbstractAction. The
// property-based accessors of Action therefore cannot serve it (it takes no
// runtime properties), so the name is kept in a member.
class UIAction: public Action {
  std::string name;

public:
  UIAction(std::string const &name) :
      name(name) {
  }

public:
  virtual std::string get_name() const override {
    return this->name;
  }

  virtual void set_name(std::string const &name) override {
    this->name = name;
  }

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
