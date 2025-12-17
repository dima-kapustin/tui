#pragma once

#include <tui++/AbstractButton.h>

namespace tui {

class Button: public AbstractButton {
protected:
  Button(std::string const& text);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

}


