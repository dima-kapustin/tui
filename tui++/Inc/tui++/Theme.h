#pragma once

#include <memory>

namespace tui {

class Themable;

class Theme {
public:
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
