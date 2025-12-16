#pragma once

#include <tui++/Event.h>
#include <tui++/Object.h>

#include <tui++/event/EventSource.h>

#include <memory>

namespace tui {

class ButtonGroup;

class ButtonModel: public Object, public EventSource<ChangeEvent, ItemEvent, ActionEvent>, public std::enable_shared_from_this<ButtonModel> {
  std::weak_ptr<ButtonGroup> group;

  struct {
    unsigned is_selected :1;
  } state;

public:
  std::shared_ptr<ButtonGroup> get_group() const {
    return this->group.lock();
  }

  void set_group(std::shared_ptr<ButtonGroup> const &group) {
    this->group = group;
  }

  bool is_selected() const {
    return this->state.is_selected;
  }

  void set_selected(bool value) {
    if (value != this->state.is_selected) {
      this->state.is_selected = value;

      fire_event<ItemEvent>(shared_from_this(), value ? ItemEvent::SELECTED : ItemEvent::DESELECTED);
    }
  }
};

}
