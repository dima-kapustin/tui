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
};

}
