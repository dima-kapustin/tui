#pragma once

#include <tui++/ContainerOrderFocusTraversalPolicy.h>

namespace tui {

class DefaultFocusTraversalPolicy: public ContainerOrderFocusTraversalPolicy {
protected:
  bool accept(const std::shared_ptr<Component> &candidate) const override;

  friend class Component;
};

}
