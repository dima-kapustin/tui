#pragma once

namespace tui {

class Menu;

struct MenuEvent {
  enum Id : unsigned {
    SELECTED,
    DESELECTED,
    CANCELLED
  };

  const std::shared_ptr<Menu> source;
  const Id id;

  constexpr MenuEvent(const std::shared_ptr<Menu> &source, Id id) :
      source(source), id(id) {
  }
};

}
