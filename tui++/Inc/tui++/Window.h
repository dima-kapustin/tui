#pragma once

#include <tui++/Component.h>

namespace tui {

class Screen;

class Window: public Component {
  using base = Component;

  Screen &screen;
  std::shared_ptr<Window> owner;

protected:
  void paint_components(Graphics &g) override;

public:
  Window(Screen &screen) :
      screen(screen) {
  }

  Window(const std::shared_ptr<Window> &owner) :
      screen(owner->screen), owner(owner) {
  }

public:
  Screen* get_screen() const override {
    return &this->screen;
  }

  bool is_displayable() const override {
    return true;
  }

  void paint(Graphics &g) override {
    validate();
    base::paint(g);
    request_focus();
  }
};

}
