#include <tui++/Object.h>

namespace tui {

void Object::fire_property_change_event(const std::string_view &property_name, const PropertyValue &old_value, const PropertyValue &new_value) {
  if (not this->property_change_listeners.empty()) {
    for (auto &&name : { property_name, std::string_view { ANY_PROPERTY_NAME } }) {
      auto listeners_pos = std::lower_bound(this->property_change_listeners.begin(), this->property_change_listeners.end(), name, //
          [](const auto &a, const std::string_view &name) {
            return a.first < name;
          });

      if (listeners_pos != this->property_change_listeners.end() and listeners_pos->first == name) {
        auto event = PropertyChangeEvent { this, name, old_value, new_value };
        for (auto &&listener : listeners_pos->second) {
          listener(event);
        }
      }
    }

  }
}

}
