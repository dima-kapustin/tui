#pragma once

#include <tui++/Window.h>

namespace tui {
namespace laf {
class FrameUI;
}

class MenuBar;

class Frame: public Window {
  using base = Window;

  Frame(Screen &screen) :
      Window(screen) {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  Property<std::string> title { this, "title" };

protected:
  void init() override;

  std::shared_ptr<laf::ComponentUI> create_ui() override;

public:
  const std::string& get_title() const {
    return this->title;
  }

  void set_title(const std::string &title) {
    this->title = title;
  }

  std::shared_ptr<MenuBar> get_menu_bar() const;
  void set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);

  std::shared_ptr<laf::FrameUI> get_ui() const;
};

}
