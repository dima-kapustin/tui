#include <tui++/Object.h>

namespace tui {

void Object::fire_property_change_event(const std::string_view &property_name, const PropertyValue &old_value, const PropertyValue &new_value) {
  if (not this->property_change_listeners.empty()) {
    // Listeners registered for the property itself and the ones registered
    // for any property (see add_property_change_listener) are told the same
    // thing: the name of the property that changed. The wildcard bucket is
    // looked up by "*", but a listener must not have to know it matched
    // through a wildcard -- AbstractButton, for instance, follows the
    // enabled state of its action by checking for "enabled" among the events
    // of a wildcard listener.
    for (auto &&lookup : { property_name, std::string_view { ANY_PROPERTY_NAME } }) {
      auto listeners_pos = std::lower_bound(this->property_change_listeners.begin(), this->property_change_listeners.end(), lookup, //
          [](const auto &a, const std::string_view &name) {
            return a.first < name;
          });

      if (listeners_pos != this->property_change_listeners.end() and listeners_pos->first == lookup) {
        auto event = PropertyChangeEvent { this, property_name, old_value, new_value };
        for (auto &&listener : listeners_pos->second) {
          listener(event);
        }
      }
    }
  }
}

}
