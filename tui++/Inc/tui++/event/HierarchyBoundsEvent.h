#pragma once

#include <tui++/event/HierarchyEvent.h>

namespace tui {

class HierarchyBoundsEvent: public BasicHierarchyEvent {
public:
  enum Type : unsigned {
    ANCESTOR_MOVED = event_id_v<EventType::HIERARCHY_BOUNDS, 0>,
    ANCESTOR_RESIZED = event_id_v<EventType::HIERARCHY_BOUNDS, 1>
  };

public:
  HierarchyBoundsEvent(const std::shared_ptr<Component> &source, Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) :
      BasicHierarchyEvent(source, type, changed, changed_parent) {
  }
};

}
