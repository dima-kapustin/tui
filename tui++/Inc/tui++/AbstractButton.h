#pragma once

#include <tui++/Component.h>
#include <tui++/event/ItemEvent.h>

namespace tui {

class AbstractButton: public Component, public BasicEventSource<ItemEvent> {

protected:
  void process_event(FocusEvent &e) override {

  }

  void process_event(ItemEvent &e) override {

  }
};

}

