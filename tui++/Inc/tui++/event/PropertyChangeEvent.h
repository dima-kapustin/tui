#pragma once

#include <any>
#include <functional>
#include <string_view>

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

}
