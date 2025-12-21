#pragma once

namespace tui {

class PopupMenu;

struct PopupMenuEvent {
  enum Id : unsigned {
    BECOMES_VISIBLE,
    BECOMES_INVISIBLE,
    CANCELLED
  };

  const std::shared_ptr<PopupMenu> source;
  const Id id;

  constexpr PopupMenuEvent(const std::shared_ptr<PopupMenu> &source, Id id) :
      source(source), id(id) {
  }
};

}
