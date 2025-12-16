#pragma once

#include <tui++/Object.h>
#include <tui++/MenuElement.h>

#include <tui++/event/ChangeEvent.h>
#include <tui++/event/EventSource.h>

#include <memory>
#include <vector>

namespace tui {

struct Point;
class Component;

class MenuSelectionManager: virtual public Object, public std::enable_shared_from_this<MenuSelectionManager>, public EventSource<ChangeEvent> {
  std::unique_ptr<ChangeEvent> change_event;
  std::vector<std::shared_ptr<MenuElement>> selection;

private:
  void fire_state_changed() {
    if (not this->change_event) {
      this->change_event = std::make_unique<ChangeEvent>(shared_from_this());
    }
    fire_event(*this->change_event);
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

  void process_mouse_event(MouseEvent &e);
  void process_key_event(KeyEvent &e);

  std::shared_ptr<Component> component_for_point(const std::shared_ptr<Component> &source, const Point &source_point);
};

}
