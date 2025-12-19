#pragma once

#include <tui++/MenuItem.h>
#include <tui++/PopupMenu.h>

#include <tui++/event/MenuEvent.h>

#include <chrono>

namespace tui {
namespace laf {
class MenuUI;
}

class PopupMenu;

class Menu: public ComponentExtension<MenuItem, MenuEvent> {
  using base = ComponentExtension<MenuItem, MenuEvent>;

  std::shared_ptr<PopupMenu> popup_menu;
  std::chrono::milliseconds delay;

  bool is_selected = false;
  std::optional<Point> custom_menu_location;

public:
  std::shared_ptr<laf::MenuUI> get_ui() const;
  virtual void update_ui() override;

  std::chrono::milliseconds get_delay() const {
    return this->delay;
  }

  void set_delay(std::chrono::milliseconds const &delay) {
    this->delay = delay;
  }

  bool is_popup_menu_visible() const;
  void set_popup_menu_visible(bool value);

  void set_menu_location(Point const &p);

protected:
  Menu(std::string const &text = "") :
      base(text) {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual void init() override;
  virtual std::shared_ptr<laf::ComponentUI> create_ui() override;

  virtual void init_focusability() override;
  virtual void state_changed(ChangeEvent &e) override;

  Point get_popup_menu_origin() const;
};

}
