#include <tui++/Object.h>

namespace tui {

void Object::fire_property_change_event(const std::string_view &property_name, const PropertyValue &old_value, const PropertyValue &new_value) {
  if (not this->property_change_listeners.empty()) {
    if (auto pos = this->property_change_listeners.find(property_name); pos != this->property_change_listeners.end()) {
      for (auto &&listener : pos->second) {
        listener(this, property_name, old_value, new_value);
      }
    }
  }
}

}
