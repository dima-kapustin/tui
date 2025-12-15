#pragma once

#include <any>
#include <ranges>
#include <vector>
#include <cassert>
#include <cstring>
#include <variant>
#include <concepts>
#include <algorithm>
#include <functional>

namespace tui {

class Object;

using PropertyValue = std::any;
struct PropertyChangeEvent {
  Object *const source;
  const std::string_view &property_name;
  const PropertyValue &old_value;
  const PropertyValue &new_value;

  constexpr PropertyChangeEvent(Object *source, const std::string_view &property_name, const PropertyValue &old_value, const PropertyValue &new_value) :
      source(source), property_name(property_name), old_value(old_value), new_value(new_value) {
  }
};

using PropertyChangeListenerSignature = void(PropertyChangeEvent& e);
using PropertyChangeListener = std::function<PropertyChangeListenerSignature>;

class PropertyBase {
  Object *object;
  const char *name;

  friend class Object;
protected:
  PropertyBase(Object *object, const char *name);

protected:
  void fire_change_event(const PropertyValue &old_value, const PropertyValue &new_value);
};

class Object {
  std::vector<PropertyBase*> properties;
  mutable std::vector<std::pair<std::string_view, std::vector<PropertyChangeListener>>> property_change_listeners;

private:
  auto add_property(PropertyBase *property) {
    auto pos = std::lower_bound(this->properties.begin(), this->properties.end(), property, //
        [](const auto &a, const auto &b) {
          return std::strcmp(a->name, b->name) < 0;
        });
    this->properties.insert(pos, property);
  }

  void fire_property_change_event(const std::string_view &property_name, const PropertyValue &old_value, const PropertyValue &new_value);

  friend class PropertyBase;

public:
  auto get_properties() {
    return this->properties;
  }

  auto get_properties() const {
    return this->properties;
  }

  PropertyBase* get_property(const char *property_name) {
    auto pos = std::lower_bound(this->properties.begin(), this->properties.end(), property_name, //
        [](const auto &a, const char *property_name) {
          return std::strcmp(a->name, property_name) < 0;
        });
    return pos == this->properties.end() or std::strcmp((*pos)->name, property_name) != 0 ? nullptr : *pos;
  }

  const PropertyBase* get_property(const char *property_name) const {
    auto pos = std::lower_bound(this->properties.begin(), this->properties.end(), property_name, //
        [](const auto &a, const char *property_name) {
          return std::strcmp(a->name, property_name) < 0;
        });
    return pos == this->properties.end() or std::strcmp((*pos)->name, property_name) != 0 ? nullptr : *pos;
  }

  void add_property_change_listener(const char *property_name, const PropertyChangeListener &listener) const {
    auto listeners_pos = std::lower_bound(this->property_change_listeners.begin(), this->property_change_listeners.end(), property_name, //
        [](const auto &a, const char *property_name) {
          return a.first < property_name;
        });

    if (listeners_pos == this->property_change_listeners.end() or listeners_pos->first != property_name) {
      listeners_pos = this->property_change_listeners.insert(listeners_pos, std::make_pair(property_name, std::vector<PropertyChangeListener> { }));
    }

    listeners_pos->second.emplace_back(listener);
  }

  void remove_property_change_listener(const char *property_name, const PropertyChangeListener &listener) const {
    if (not this->property_change_listeners.empty()) {
      auto listeners_pos = std::lower_bound(this->property_change_listeners.begin(), this->property_change_listeners.end(), property_name, //
          [](const auto &a, const char *property_name) {
            return a.first < property_name;
          });

      if (listeners_pos != this->property_change_listeners.end() and listeners_pos->first == property_name) {
        auto &&change_listeners = listeners_pos->second;
        auto pos = std::find_if(change_listeners.begin(), change_listeners.end(), //
            [&listener](const PropertyChangeListener &e) {
              return e.target<PropertyChangeListenerSignature>() == listener.target<PropertyChangeListenerSignature>();
            });
        if (pos != change_listeners.end()) {
          change_listeners.erase(pos);
        }
      }
    }
  }

  void add_property_change_listener(const PropertyChangeListener &listener) const {
    add_property_change_listener("*", listener);
  }

  void remove_property_change_listener(const PropertyChangeListener &listener) const {
    remove_property_change_listener("*", listener);
  }
};

inline PropertyBase::PropertyBase(Object *object, const char *name) :
    object(object), name(name) {
  this->object->add_property(this);
}

inline void PropertyBase::fire_change_event(const PropertyValue &old_value, const PropertyValue &new_value) {
  this->object->fire_property_change_event(this->name, old_value, new_value);
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

template<typename, typename = void>
struct has_member_access_operator: std::false_type {
};

template<typename T>
struct has_member_access_operator<T, std::void_t<decltype(std::declval<T>().operator ->())>> : std::true_type {
};

template<typename T>
constexpr bool has_member_access_operator_v = has_member_access_operator<T>::value;

}

template<typename T, typename = void>
class Property;

template<typename T>
class Property<T, std::enable_if_t<not detail::is_optional_v<T>>> : public PropertyBase {
  T value_;

private:
  void set_value(const T &value) {
    if (this->value_ != value) {
      auto old_value = this->value_;
      this->value_ = value;
      fire_change_event(old_value, this->value_);
    }
  }

public:
  constexpr Property(Object *object, const char *name, const T &default_value = T()) :
      PropertyBase(object, name), value_(default_value) {
  }

public:
  PropertyValue get_value() const {
    return this->value_;
  }

  void set_value(const PropertyValue &value) {
    set_value(*std::any_cast<T*>(&value));
  }

  operator const T&() const {
    return value();
  }

  const T& value() const {
    return this->value_;
  }

  Property& operator=(const T &value) {
    set_value(value);
    return *this;
  }

  bool operator==(const Property &other) const {
    return this->value_ == other.value;
  }

  bool operator==(const T &other) const {
    return this->value_ == other;
  }

  template<typename U = T, std::enable_if_t<detail::has_bool_operator_v<U>, bool> = true>
  operator bool() const {
    return bool(this->value_);
  }

  template<typename U = T, std::enable_if_t<detail::has_member_access_operator_v<U>, bool> = true>
  U& operator ->() {
    return this->value_;
  }

  template<typename U = T, std::enable_if_t<detail::has_member_access_operator_v<U>, bool> = true>
  const U& operator ->() const {
    return this->value_;
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
  constexpr Property(Object *object, const char *name, const std::optional<value_type> &default_value = std::nullopt) :
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

  bool has_value() const {
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
