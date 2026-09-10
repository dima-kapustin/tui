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
  // The keys are owned: a key may be built at runtime (the button family
  // installs its colors under "<Kind>.BackgroundColor"), and a string_view
  // key would dangle as soon as the temporary it points into is gone.
  // Lookups still take a string_view through the transparent hash, so
  // resolving a property neither allocates nor copies.
  struct KeyHash {
    using is_transparent = void;

    size_t operator()(std::string_view const &key) const {
      return std::hash<std::string_view> { }(key);
    }
  };

  mutable std::unordered_map<std::string, std::any, KeyHash, std::equal_to<>> properties;
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
      // A theme property may hold the optional itself: the component defaults
      // are built from get_color() results, which are optionals. Reading the
      // key back then has to yield the optional, not the missing value it
      // would be mistaken for when only the plain value type is looked for.
      if (auto *value = std::any_cast<T>(&pos->second)) {
        return *value;
      }
      if (auto *value = std::any_cast<typename T::value_type>(&pos->second)) {
        return *value;
      }
    }
    return default_value;
  }

  template<typename T>
  void put(std::string_view const &key, T &&value) {
    if constexpr (std::is_invocable_v<T>) {
      this->properties.insert_or_assign(std::string(key), std::function { std::forward<T>(value) });
    } else {
      this->properties.insert_or_assign(std::string(key), std::forward<T>(value));
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
        this->properties.insert_or_assign(std::string(key), new_value);
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
