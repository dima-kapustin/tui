#pragma once

#include <tui++/Component.h>

namespace tui {

class MenuBar: public Component {
protected:
  MenuBar(std::string const& text);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

}
