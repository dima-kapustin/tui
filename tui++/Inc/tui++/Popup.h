#pragma once

#include <memory>

namespace tui {

class Window;
class Component;

class Popup {
  std::shared_ptr<Window> window;
public:
  Popup(std::shared_ptr<Component> const& owner, std::shared_ptr<Component> const& contents, int owner_x, int owner_y);

public:
  void show();
  void hide();

  // The popup window this popup shows its contents in: a popup menu watches
  // it to notice its window going off the screen (see PopupMenu::drop_popup).
  std::shared_ptr<Window> get_window() const {
    return this->window;
  }
};

}
