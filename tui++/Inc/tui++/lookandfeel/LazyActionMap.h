#pragma once

#include <tui++/ActionMap.h>

#include <functional>
#include <type_traits>

namespace tui::laf {

class LazyActionMap: public ActionMap {
  using base = ActionMap;

  std::function<void(LazyActionMap&)> loader;
public:
  template<typename Loader, std::enable_if_t<std::is_invocable_v<Loader, LazyActionMap&>, bool> = true>
  LazyActionMap(Loader &&loader) :
      loader(std::forward<Loader>(loader)) {
  }

public:
  virtual std::vector<ActionKey> get_keys() const {
    load_if_necessary();
    return base::get_keys();
  }

  virtual std::vector<ActionKey> get_all_keys() const {
    load_if_necessary();
    return base::get_all_keys();
  }

  virtual std::shared_ptr<Action> at(const ActionKey &key) const override {
    load_if_necessary();
    return base::at(key);
  }

  virtual void set_parent(std::shared_ptr<ActionMap> const &parent) {
    load_if_necessary();
    base::set_parent(parent);
  }

  void emplace(std::shared_ptr<Action> const &action) {
    emplace(action->get_name(), action);
  }

  virtual void emplace(const ActionKey &key, const ActionListener &action_listener) override {
    load_if_necessary();
    base::emplace(key, action_listener);
  }

  virtual void emplace(const ActionKey &key, const std::shared_ptr<Action> &action) {
    load_if_necessary();
    base::emplace(key, action);
  }

  virtual void erase(const ActionKey &key) {
    load_if_necessary();
    base::erase(key);
  }

private:
  void load_if_necessary() const {
    const_cast<LazyActionMap*>(this)->load_if_necessary();
  }

  void load_if_necessary() {
    if (auto loader = this->loader) {
      this->loader = nullptr;
      loader(*this);
    }
  }
};

}
