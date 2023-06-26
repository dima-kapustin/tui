#pragma once

#include <tui++/Window.h>

namespace tui {

/**
 * A Frame is a top-level window with a title and a border.
 */
class Frame: public Window {
  Frame(Screen &screen) :
      Window(screen) {
    init();
  }

  friend class Screen;

private:
  void init() {
    enable_events(KEY_EVENT_MASK | WINDOW_EVENT_MASK);
  }

public:
  void paint(Graphics &g) override;

  void set_title(const std::string &title);
};

}
