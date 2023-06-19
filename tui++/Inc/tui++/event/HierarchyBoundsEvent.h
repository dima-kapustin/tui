#pragma once

#include <tui++/event/HierarchyEvent.h>

namespace tui {

class HierarchyBoundsEvent: public BasicHierarchyEvent {
public:
  enum Type {
    ANCESTOR_MOVED,
    ANCESTOR_RESIZED
  };

public:
  const Type type;

public:
  HierarchyBoundsEvent(const std::shared_ptr<Component> &source, Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) :
      BasicHierarchyEvent(source, changed, changed_parent), type(type) {
  }
};

}
