#pragma once

#include <tui++/event/BasicEvent.h>

namespace tui {

class BasicHierarchyEvent: public BasicEvent {
public:
  /**
   * The Component at the top of the hierarchy which was changed.
   */
  std::shared_ptr<Component> changed;
  /**
   * The parent of the changed component. This may be the parent
   * before or after the change, depending on the type of change.
   */
  std::shared_ptr<Component> changed_parent;

protected:
  BasicHierarchyEvent(const std::shared_ptr<Component> &source, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) :
      BasicEvent(source), changed(changed), changed_parent(changed_parent) {
  }
};

class HierarchyEvent: public BasicHierarchyEvent {
public:
  enum Type {
    PARENT_CHANGED,
    DISPLAYABILITY_CHANGED,
    SHOWING_CHANGED
  };

public:
  const Type type;

public:
  HierarchyEvent(const std::shared_ptr<Component> &source, Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) :
      BasicHierarchyEvent(source, changed, changed_parent), type(type) {
  }
};

}
