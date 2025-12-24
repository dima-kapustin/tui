#pragma once

#include <tui++/Window.h>

namespace tui {
namespace laf {
class FrameUI;
}

class MenuBar;

class Frame: public Window {
  using base = Window;

  Frame() {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  void init() override;

  std::shared_ptr<laf::ComponentUI> create_ui() override;

public:
  std::shared_ptr<MenuBar> get_menu_bar() const;
  void set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);

  std::shared_ptr<laf::FrameUI> get_ui() const;
};

}
