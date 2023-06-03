#pragma once

#include <any>
#include <map>
#include <ranges>
#include <cassert>
#include <variant>
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

template<typename T>
class Property: public PropertyBase {
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
  constexpr Property(Object *object, const std::string_view &name) :
      PropertyBase(object, name) {
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

}
