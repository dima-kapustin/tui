#pragma once

#include <any>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace tui {

class Theme;
class Themable;

namespace laf {
class LookAndFeel;
}

template<typename T>
constexpr bool is_theme_v = std::is_base_of_v<Theme, T>;

class Theme {
  std::unordered_map<std::string_view, std::any> properties;
public:
  virtual ~Theme() = default;

public:
  template<typename T>
  T get(std::string_view const &key, T &&default_value = { }) {
    if (auto pos = this->properties.find(key); pos != this->properties.end()) {
      if (auto *value = std::any_cast<T>(&pos->second)) {
        return *value;
      }
    }
    return default_value;
  }

  template<typename T>
  void put(std::string_view const &key, T &&value) {
    this->properties.emplace(key, std::forward<T>(value));
  }

  void put(std::initializer_list<std::pair<std::string_view, std::any>> &&values) {
    for (auto&& [key, value] : values) {
      this->properties.emplace(std::move(key), std::move(value));
    }
  }

  template<typename T, typename ... Args>
  std::shared_ptr<T> make_resource(Args &&... args) {
    auto resource = std::make_shared<T>(std::forward<Args>(args)...);
    resource->theme = this;
    return resource;
  }

  template<typename T>
  constexpr std::enable_if_t<std::is_base_of_v<Themable, T>, T&&> make_resource(T &&obj) {
    obj.theme = this;
    return std::move(obj);
  }

protected:
  Theme() = default;

protected:
  virtual void install() = 0;
  virtual void uninstall() {
  }

  friend class laf::LookAndFeel;
};

constexpr bool is_theme_resource(Themable const&);
constexpr bool is_theme_resource(Themable const*);
constexpr bool is_theme_resource(std::shared_ptr<Themable const> const&);
constexpr bool is_theme_resource(std::unique_ptr<Themable const> const&);

class Themable {
  Theme *theme = nullptr;

  friend constexpr bool is_theme_resource(Themable const&);
  friend constexpr bool is_theme_resource(Themable const*);
  friend constexpr bool is_theme_resource(std::shared_ptr<Themable const> const&);
  friend constexpr bool is_theme_resource(std::unique_ptr<Themable const> const&);

  friend class Theme;

public:
  constexpr virtual ~Themable() {
  }

  constexpr bool operator==(Themable const &other) const = delete;

protected:
  constexpr Themable() {
  }
};

constexpr bool is_theme_resource(Themable const &obj) {
  return obj.theme;
}

constexpr bool is_theme_resource(Themable const *obj) {
  return obj and is_theme_resource(*obj);
}

constexpr bool is_theme_resource(std::shared_ptr<Themable const> const &obj) {
  return obj and is_theme_resource(*obj);
}

constexpr bool is_theme_resource(std::unique_ptr<Themable const> const &obj) {
  return obj and is_theme_resource(*obj);
}

}
