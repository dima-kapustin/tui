#pragma once

#include <vector>
#include <memory>
#include <cstring>
#include <concepts>
#include <algorithm>

#include <tui++/event/PropertyChangeEvent.h>

namespace tui {

class PropertyBase {
  Object *const object;
  const char *name;

  friend class Object;
protected:
  PropertyBase(Object *object, const char *name);

protected:
  void fire_change_event(const PropertyValue &old_value, const PropertyValue &new_value);

public:
  virtual ~PropertyBase() {
  }

  virtual PropertyValue get_value() const = 0;
  virtual void set_value(const PropertyValue&) = 0;

  void add_change_listener(const PropertyChangeListener &listener) const;
  void remove_change_listener(const PropertyChangeListener &listener) const;
};

template<typename T, typename = void>
class Property;

class Object {
  std::vector<PropertyBase*> properties;
  std::vector<std::unique_ptr<PropertyBase>> runtime_properties;
  mutable std::vector<std::pair<std::string_view, std::vector<PropertyChangeListener>>> property_change_listeners;

  constexpr static auto ANY_PROPERTY_NAME = "*";

private:
  [[maybe_unused]]
  PropertyBase* add_property(PropertyBase *property) {
    auto pos = std::lower_bound(this->properties.begin(), this->properties.end(), property, //
        [](const auto &a, const auto &b) {
          return std::strcmp(a->name, b->name) < 0;
        });
    return *this->properties.insert(pos, property);
  }

  template<typename T>
  Property<T>* add_runtime_property(const char *name) {
    return static_cast<Property<T>*>(add_runtime_property(std::make_unique<Property<T>>(this, name)));
  }

  void fire_property_change_event(const std::string_view &property_name, const PropertyValue &old_value, const PropertyValue &new_value);

  friend class PropertyBase;

protected:
  virtual ~Object() {
  }

  virtual PropertyBase* add_runtime_property(std::unique_ptr<PropertyBase> &&property) {
    return add_property(this->runtime_properties.emplace_back(std::move(property)).get());
  }

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

  template<typename ValueType>
  PropertyBase* add_property(const char *name) {
    return get_property(name) ? nullptr : add_runtime_property<ValueType>(name);
  }

  void remove_property(const char *name) {
    for (auto it = this->runtime_properties.begin(); it != this->runtime_properties.end(); ++it) {
      if (std::strcmp((*it)->name, name) == 0) {
        this->runtime_properties.erase(it);
        break;
      }
    }
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
    add_property_change_listener(ANY_PROPERTY_NAME, listener);
  }

  void remove_property_change_listener(const PropertyChangeListener &listener) const {
    remove_property_change_listener("*", listener);
  }

  PropertyValue get_property_value(const char *property_name) const {
    if (auto *property = get_property(property_name)) {
      return property->get_value();
    }
    return {};
  }

  template<typename ValueType>
  ValueType get_property_value(const char *property_name) const {
    if (auto *property = get_property(property_name)) {
      auto any = property->get_value();
      if (auto *value = std::any_cast<ValueType>(&any)) {
        return std::move(*value);
      }
    }
    return {};
  }

  template<typename ValueType>
  void set_property_value(const char *property_name, const ValueType &value) {
    if (auto *property = get_property(property_name)) {
      property->set_value(value);
    } else {
      *add_runtime_property<std::remove_cvref_t<ValueType>>(property_name) = value;
    }
  }
};

inline PropertyBase::PropertyBase(Object *object, const char *name) :
    object(object), name(name) {
  this->object->add_property(this);
}

inline void PropertyBase::fire_change_event(const PropertyValue &old_value, const PropertyValue &new_value) {
  this->object->fire_property_change_event(this->name, old_value, new_value);
}

inline void PropertyBase::add_change_listener(const PropertyChangeListener &listener) const {
  this->object->add_property_change_listener(this->name, listener);
}

inline void PropertyBase::remove_change_listener(const PropertyChangeListener &listener) const {
  this->object->remove_property_change_listener(this->name, listener);
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
  PropertyValue get_value() const override final {
    return this->value_;
  }

  void set_value(const PropertyValue &value) override final {
    set_value(*std::any_cast<T>(&value));
  }

  operator const T&() const {
    return value();
  }

  const T& value() const {
    return this->value_;
  }

  Property& operator=(const Property &other) {
    set_value(other.value);
    return *this;
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
  PropertyValue get_value() const override final {
    return this->optional.has_value() ? this->optional.value() : PropertyValue { };
  }

  void set_value(const PropertyValue &value) override final {
    if (value.has_value()) {
      set_optional_value(std::nullopt);
    } else {
      set_optional_value(std::make_optional<value_type>(*std::any_cast<value_type>(&value)));
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

  Property& operator=(const Property &other) {
    set_optional_value(other.optional);
    return *this;
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

  bool operator==(const value_type &other) const {
    return this->optional.has_value() and this->optional.value() == other;
  }
};

template<typename Derived>
constexpr bool is_a(Object const *obj) {
  return dynamic_cast<std::decay_t<Derived> const*>(obj) != nullptr;
}

template<typename Derived, typename Base>
constexpr bool is_a(std::shared_ptr<Base const> const &ptr) {
  return dynamic_cast<std::decay_t<Derived> const*>(ptr.get()) != nullptr;
}

template<typename Derived, typename Base>
constexpr bool is_a(std::shared_ptr<Base> const &ptr) {
  return dynamic_cast<std::decay_t<Derived>*>(ptr.get()) != nullptr;
}

}
