#pragma once

#include <tui++/Object.h>
#include <tui++/event/ChangeEvent.h>
#include <tui++/event/EventSource.h>

#include <limits>
#include <memory>

namespace tui {

class SingleSelectionModel: public Object, public EventSource<ChangeEvent>, public std::enable_shared_from_this<SingleSelectionModel> {
  size_t index;
public:
  constexpr static auto NO_SELECTION = std::numeric_limits<size_t>::max();

  size_t get_selected_index() const {
    return this->index;
  }

  void set_selected_index(size_t index) {
    if (this->index != index) {
      this->index = index;
      fire_event<ChangeEvent>(shared_from_this());
    }
  }

  void clear_selection() {
    set_selected_index(NO_SELECTION);
  }

  bool is_selected() const {
    return this->index != NO_SELECTION;
  }
};

}
