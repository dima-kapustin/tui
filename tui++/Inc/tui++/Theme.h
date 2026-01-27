#pragma once

#include <any>
#include <memory>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <tui++/Color.h>

#include <tui++/util/type_traits.h>

namespace tui {

class Icon;
class Border;

class Theme;
class Themable;

namespace laf {
class LookAndFeel;
}

template<typename T>
constexpr bool is_theme_v = std::is_base_of_v<Theme, T>;

class Theme {
  mutable std::unordered_map<std::string_view, std::any> properties;
public:
  virtual ~Theme() = default;

public:
  using BorderFactory = std::function<std::shared_ptr<Border>()>;
  using IconFactory = std::function<std::shared_ptr<Icon>()>;

public:
  template<typename T>
  std::enable_if_t<not util::is_optional_v<T>, T> get(std::string_view const &key, T &&default_value = { }) const {
    if (auto pos = this->properties.find(key); pos != this->properties.end()) {
      if (auto *value = std::any_cast<T>(&pos->second)) {
        return *value;
      }
    }
    return default_value;
  }

  template<typename T>
  std::enable_if_t<util::is_optional_v<T>, T> get(std::string_view const &key, T &&default_value = std::nullopt) const {
    if (auto pos = this->properties.find(key); pos != this->properties.end()) {
      if (auto *value = std::any_cast<typename T::value_type>(&pos->second)) {
        return *value;
      }
    }
    return default_value;
  }

  template<typename T>
  void put(std::string_view const &key, T &&value) {
    if constexpr (std::is_invocable_v<T>) {
      this->properties.insert_or_assign(key, std::function { std::forward<T>(value) });
    } else {
      this->properties.insert_or_assign(key, std::forward<T>(value));
    }
  }

  void put(std::initializer_list<std::pair<std::string_view, std::any>> &&values) {
    for (auto&& [key, value] : values) {
      put(key, std::move(value));
    }
  }

  template<typename T, typename ... Args>
  std::enable_if_t<std::is_base_of_v<Themable, T>, std::shared_ptr<T>> make_shared_resource(Args &&... args) {
    auto resource = std::make_shared<T>(std::forward<Args>(args)...);
    resource->theme = this;
    return resource;
  }

  template<typename T, typename ... Args>
  std::enable_if_t<std::is_base_of_v<Themable, T>, T> make_resource(Args &&... args) {
    auto resource = T { std::forward<Args>(args)... };
    resource.theme = this;
    return resource;
  }

  template<typename T>
  constexpr std::enable_if_t<std::is_base_of_v<Themable, T>, T&&> make_resource(T &&obj) {
    obj.theme = this;
    return std::move(obj);
  }

  template<typename T>
  constexpr std::enable_if_t<std::is_base_of_v<Themable, T>, T> make_resource(T const &obj) {
    auto copy = obj;
    copy.theme = this;
    return copy;
  }

  std::shared_ptr<Icon> get_icon(std::string_view const &key) const {
    return get_lazy<Icon>(key);
  }

  std::shared_ptr<Border> get_border(std::string_view const &key) const {
    return get_lazy<Border>(key);
  }

  std::optional<Color> get_color(std::string_view const &key) const {
    return get<std::optional<Color>>(key);
  }

protected:
  Theme() = default;

  template<typename T>
  std::shared_ptr<T> get_lazy(std::string_view const &key) const {
    if (auto pos = this->properties.find(key); pos != this->properties.end()) {
      if (auto *value = std::any_cast<std::shared_ptr<T>>(&pos->second)) {
        return *value;
      } else if (auto *factory = std::any_cast<std::function<std::shared_ptr<T>()>>(&pos->second)) {
        auto new_value = (*factory)();
        this->properties.insert_or_assign(key, new_value);
        return new_value;
      }
    }
    return {};
  }

protected:
  virtual void init() = 0;
  virtual void deinit() {
  }

  friend class laf::LookAndFeel;
};

}
