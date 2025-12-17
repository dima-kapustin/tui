#pragma once

#include <tui++/MenuItem.h>

namespace tui {

class Menu: public MenuItem {
protected:
  Menu(std::string const& text);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

}
