#include <tui++/Object.h>

namespace tui {

void Object::fire_property_change_event(const std::string_view &property_name, const PropertyValue &old_value, const PropertyValue &new_value) {
  if (not this->property_change_listeners.empty()) {
    auto listeners_pos = std::lower_bound(this->property_change_listeners.begin(), this->property_change_listeners.end(), property_name, //
        [](const auto &a, const std::string_view &property_name) {
          return a.first < property_name;
        });

    if (listeners_pos != this->property_change_listeners.end() and listeners_pos->first == property_name) {
      auto event = PropertyChangeEvent { this, property_name, old_value, new_value };
      for (auto &&listener : listeners_pos->second) {
        listener(event);
      }
    }
  }
}

}
