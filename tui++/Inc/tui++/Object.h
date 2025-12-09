#pragma once

#include <any>
#include <ranges>
#include <vector>
#include <cassert>
#include <variant>
#include <concepts>
#include <algorithm>
#include <functional>
#include <string_view>

namespace tui {

class Object;

using PropertyValue = std::any;
using PropertyChangeListenerSignature = void(Object*, const std::string_view &property_name, const PropertyValue& old_value, const PropertyValue &new_value);
using PropertyChangeListener = std::function<PropertyChangeListenerSignature>;

class PropertyBase {
  Object *object;
  std::string_view name;

  mutable std::vector<PropertyChangeListener> change_listeners;

  friend class Object;

protected:
  void fire_change_event(const PropertyValue &old_value, const PropertyValue &new_value) {
    for (auto &&change_listener : this->change_listeners) {
      change_listener(this->object, this->name, old_value, new_value);
    }
  }

protected:
  PropertyBase(Object *object, const std::string_view &name);

public:
  void add_change_listener(const PropertyChangeListener &listener) const {
    auto pos = std::find_if(this->change_listeners.begin(), this->change_listeners.end(), //
        [&listener](const PropertyChangeListener &e) {
          return e.target<PropertyChangeListenerSignature>() == listener.target<PropertyChangeListenerSignature>();
        });
    if (pos == this->change_listeners.end()) {
      this->change_listeners.emplace_back(listener);
    }
  }

  void remove_change_listener(const PropertyChangeListener &listener) const {
    std::erase_if(this->change_listeners, [&listener](const PropertyChangeListener &e) {
      return e.target<PropertyChangeListenerSignature>() == listener.target<PropertyChangeListenerSignature>();
    });
  }
};

class Object {
  std::vector<PropertyBase*> properties;

private:
  auto add_property(PropertyBase *property) {
    auto pos = std::lower_bound(this->properties.begin(), this->properties.end(), property, //
        [property](const auto &a, const auto &b) {
          return a->name < b->name;
        });
    this->properties.insert(pos, property);
  }

  friend class PropertyBase;

public:
  auto get_properties() {
    return this->properties;
  }

  auto get_properties() const {
    return this->properties;
  }

  PropertyBase* get_property(const std::string_view &name) {
    auto pos = std::lower_bound(this->properties.begin(), this->properties.end(), name, //
        [](const auto &a, const std::string_view &name) {
          return a->name < name;
        });
    return pos == this->properties.end() or ((*pos)->name != name) ? nullptr : *pos;
  }

  const PropertyBase* get_property(const std::string_view &name) const {
    auto pos = std::lower_bound(this->properties.begin(), this->properties.end(), name, //
        [](const auto &a, const std::string_view &name) {
          return a->name < name;
        });
    return pos == this->properties.end() or ((*pos)->name != name) ? nullptr : *pos;
  }
};

inline PropertyBase::PropertyBase(Object *object, const std::string_view &name) :
    object(object), name(name) {
  this->object->add_property(this);
}

namespace detail {

template<typename, typename = void>
struct is_optional: std::false_type {
};

template<typename T>
struct is_optional<std::optional<T> > : std::true_type {
};

template<typename T>
constexpr bool is_optional_v = is_optional<T>::value;

template<typename, typename = void>
struct has_bool_operator: std::false_type {
};

template<typename T>
struct has_bool_operator<T, std::void_t<decltype(std::declval<T>().operator bool())>> : std::true_type {
};

template<typename T>
constexpr bool has_bool_operator_v = has_bool_operator<T>::value;

}

template<typename T, typename = void>
class Property;

template<typename T>
class Property<T, std::enable_if_t<not detail::is_optional_v<T>>> : public PropertyBase {
  T raw_value;

private:
  void set_value(const T &value) {
    if (this->raw_value != value) {
      auto old_value = this->raw_value;
      this->raw_value = value;
      fire_change_event(old_value, this->raw_value);
    }
  }

public:
  constexpr Property(Object *object, const std::string_view &name, const T &default_value = T()) :
      PropertyBase(object, name), raw_value(default_value) {
  }

public:
  PropertyValue get_value() const {
    return this->raw_value;
  }

  void set_value(const PropertyValue &value) {
    set_value(*std::any_cast<T*>(&value));
  }

  operator const T&() const {
    return value();
  }

  const T& value() const {
    return this->raw_value;
  }

  Property& operator=(const T &value) {
    set_value(value);
    return *this;
  }

  bool operator==(const Property &other) const {
    return this->raw_value == other.value;
  }

  bool operator==(const T &other) const {
    return this->raw_value == other;
  }

  template<typename U = T, std::enable_if_t<detail::has_bool_operator_v<U>, bool> = true>
  operator bool() const {
    return bool(this->raw_value);
  }
};

template<typename T>
class Property<T, std::enable_if_t<detail::is_optional_v<T>>> : public PropertyBase {
  using value_type = typename T::value_type;

  std::optional<value_type> optional;

private:
  void set_optional_value(const std::optional<value_type> &value) {
    if (this->optional != value) {
      auto old_value = this->optional;
      this->optional = value;
      fire_change_event(old_value, this->optional);
    }
  }

public:
  constexpr Property(Object *object, const std::string_view &name, const std::optional<value_type> &default_value = std::nullopt) :
      PropertyBase(object, name), optional(default_value) {
  }

public:
  PropertyValue get_value() const {
    return this->optional.has_value() ? this->optional.value() : std::nullopt;
  }

  void set_value(const PropertyValue &value) {
    if (value.has_value()) {
      set_optional_value(std::nullopt);
    } else {
      set_optional_value(*std::any_cast<value_type*>(&value));
    }
  }

  bool has_vlaue() const {
    return this->optional.has_value();
  }

  value_type& value() {
    return this->optional.value();
  }

  const value_type& value() const {
    return this->optional.value();
  }

  value_type value_or(const value_type &v) const {
    return this->optional.value_or(v);
  }

  value_type value_or(value_type &&v) const {
    return this->optional.value_or(std::move(v));
  }

  operator const T&() const {
    return this->optional;
  }

  Property& operator=(const std::optional<value_type> &value) {
    set_optional_value(value);
    return *this;
  }

  Property& operator=(const value_type &value) {
    set_optional_value(value);
    return *this;
  }

  bool operator==(const Property &other) const {
    return this->optional == other.optional;
  }

  bool operator==(const std::optional<value_type> &other) const {
    return this->optional == other;
  }
};

}
