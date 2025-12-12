#include <tui++/Object.h>

namespace tui {

PropertyBase::PropertyBase(Object *object, const char *name) :
    object(object), name(name) {
  this->object->add_property(this);
}

void PropertyBase::fire_change_event(const PropertyValue &old_value, const PropertyValue &new_value) {
  for (auto &&change_listener : this->change_listeners) {
    change_listener(this->object, this->name, old_value, new_value);
  }

  for (auto &&change_listener : this->object->change_listeners) {
    change_listener(this->object, this->name, old_value, new_value);
  }
}

}
