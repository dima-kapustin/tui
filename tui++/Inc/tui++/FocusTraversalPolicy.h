#pragma once

#include <memory>

namespace tui {

class Component;

class FocusTraversalPolicy {
protected:

public:
  virtual ~FocusTraversalPolicy() {
  }

public:
  virtual std::shared_ptr<Component> get_component_after(const std::shared_ptr<const Component> &container, const std::shared_ptr<const Component> &component) const = 0;

  virtual std::shared_ptr<Component> get_component_before(const std::shared_ptr<const Component> &container, const std::shared_ptr<const Component> &component) const = 0;

  virtual std::shared_ptr<Component> get_first_component(const std::shared_ptr<const Component> &container) const = 0;

  virtual std::shared_ptr<Component> get_last_component(const std::shared_ptr<const Component> &container) const = 0;

  virtual std::shared_ptr<Component> get_default_component(const std::shared_ptr<const Component> &container) const = 0;

  virtual std::shared_ptr<Component> get_initial_component(const std::shared_ptr<const Window> &window) const;
};

}
