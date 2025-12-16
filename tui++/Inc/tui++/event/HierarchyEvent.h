#pragma once

#include <tui++/event/ComponentEvent.h>

namespace tui {

class BasicHierarchyEvent: public ComponentEvent {
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
  template<typename Id>
  BasicHierarchyEvent(const std::shared_ptr<Component> &source, const Id &id, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) :
      ComponentEvent(source, id), changed(changed), changed_parent(changed_parent) {
  }
};

class HierarchyEvent: public BasicHierarchyEvent {
public:
  enum Type : unsigned {
    PARENT_CHANGED = event_id_v<EventType::HIERARCHY, 0>,
    DISPLAYABILITY_CHANGED = event_id_v<EventType::HIERARCHY, 1>,
    SHOWING_CHANGED = event_id_v<EventType::HIERARCHY, 2>
  };

public:
  HierarchyEvent(const std::shared_ptr<Component> &source, Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent) :
      BasicHierarchyEvent(source, type, changed, changed_parent) {
  }
};

}
