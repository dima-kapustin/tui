#pragma once

#include <tui++/Object.h>
#include <tui++/MenuElement.h>

#include <tui++/event/ChangeEvent.h>
#include <tui++/event/EventSource.h>

#include <memory>
#include <vector>

namespace tui {

class MenuSelectionManager: virtual public Object, public EventSource<ChangeEvent> {
  ChangeEvent change_event { this };
  std::vector<std::shared_ptr<MenuElement>> selection;

private:
  void fire_state_changed() {
    fire_event(this->change_event);
  }

public:
  std::vector<std::shared_ptr<MenuElement>> get_selection_path() const {
    return this->selection;
  }

  void set_selection_path(std::vector<std::shared_ptr<MenuElement>> const &path);

  void clear_selection_path() {
    if (not this->selection.empty()) {
      set_selection_path( { });
    }
  }

  void process_mouse_event(MouseEventBase &e);
};

}
