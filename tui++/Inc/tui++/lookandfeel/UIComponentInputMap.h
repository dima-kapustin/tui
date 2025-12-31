#pragma once

#include <tui++/ComponentInputMap.h>

#include <tui++/lookandfeel/UIResource.h>

namespace tui::laf {

class UIComponentInputMap: public ComponentInputMap, public UIResource {
public:
  using ComponentInputMap::ComponentInputMap;
};

}
