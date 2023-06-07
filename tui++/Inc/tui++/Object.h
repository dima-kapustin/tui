#pragma once

#include <any>
#include <map>
#include <ranges>
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
};

class Object {
  std::map<std::string_view, PropertyBase*> properties;

private:
  void add_property(PropertyBase *property) {
    assert(properties.contains(property->name));
    this->properties[property->name] = property;
  }

  friend class PropertyBase;

public:
  auto get_properties() {
    return std::views::values(this->properties);
  }

  auto get_properties() const {
    return std::views::values(this->properties);
  }

  PropertyBase* get_property(const std::string_view &name) {
    auto pos = this->properties.find(name);
    return pos == this->properties.end() ? nullptr : pos->second;
  }

  const PropertyBase* get_property(const std::string_view &name) const {
    auto pos = this->properties.find(name);
    return pos == this->properties.end() ? nullptr : pos->second;
  }
};

inline PropertyBase::PropertyBase(Object *object, const std::string_view &name) :
    object(object), name(name) {
  this->object->add_property(this);
}

template<typename T, typename = void>
struct is_optional: std::false_type {
};

template<typename T>
struct is_optional<std::optional<T> > : std::true_type {
};

template<typename T>
constexpr bool is_optional_v = is_optional<T>::value;

template<typename T, typename = void>
class Property;

template<typename T>
class Property<T, std::enable_if_t<not is_optional_v<T>>> : public PropertyBase {
  T value;

private:
  void set_value(const T &value) {
    if (this->value != value) {
      auto old_value = this->value;
      this->value = value;
      fire_change_event(old_value, this->value);
    }
  }

public:
  constexpr Property(Object *object, const std::string_view &name, const T &default_value = T()) :
      PropertyBase(object, name), value(default_value) {
  }

public:
  PropertyValue get_value() const {
    return this->value;
  }

  void set_value(const PropertyValue &value) {
    set_value(*std::any_cast<T*>(&value));
  }

  operator const T&() const {
    return this->value;
  }

  Property& operator=(const T &value) {
    set_value(value);
    return *this;
  }
};

template<typename T>
class Property<T, std::enable_if_t<is_optional_v<T>>> : public PropertyBase {
  using value_type = typename T::value_type;

  std::optional<value_type> value;

private:
  void set_optional_value(const std::optional<value_type> &value) {
    if (this->value != value) {
      auto old_value = this->value;
      this->value = value;
      fire_change_event(old_value, this->value);
    }
  }

public:
  constexpr Property(Object *object, const std::string_view &name, const std::optional<value_type> &default_value = std::nullopt) :
      PropertyBase(object, name), value(default_value) {
  }

public:
  PropertyValue get_value() const {
    return this->value.has_value() ? this->value.value() : std::nullopt;
  }

  void set_value(const PropertyValue &value) {
    if (value.has_value()) {
      set_optional_value(std::nullopt);
    } else {
      set_optional_value(*std::any_cast<value_type*>(&value));
    }
  }

  bool has_vlaue() const {
    return this->value.has_value();
  }

  operator const T&() const {
    return this->value;
  }

  Property& operator=(const std::optional<value_type> &value) {
    set_optional_value(value);
    return *this;
  }

  Property& operator=(const value_type &value) {
    set_optional_value(value);
    return *this;
  }
};

}
