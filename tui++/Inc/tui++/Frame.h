#pragma once

#include <tui++/Window.h>

namespace tui {

/**
 * A Frame is a top-level window with a title and a border.
 */
class Frame: public Window {
  Frame(Screen &screen) :
      Window(screen) {
  }

  friend class Screen;

public:
  void paint(Graphics &g) override;

  void set_title(const std::string &title);
};

}
