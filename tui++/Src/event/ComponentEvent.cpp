#include <tui++/event/ComponentEvent.h>
#include <tui++/Component.h>

namespace tui {

ComponentEvent::ComponentEvent(const std::shared_ptr<Component> &source, unsigned id, const EventClock::time_point &when) :
    Event(source, id, when) {
}

ComponentEvent::ComponentEvent(const std::shared_ptr<Component> &source, Type type, const EventClock::time_point &when) :
    Event(source, type, when) {
}

std::shared_ptr<Component> ComponentEvent::component() const {
  return std::static_pointer_cast<Component>(this->source);
}

}
