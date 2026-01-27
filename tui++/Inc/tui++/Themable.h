#pragma once

#include <memory>

namespace tui {

class Theme;
class Themable;

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
