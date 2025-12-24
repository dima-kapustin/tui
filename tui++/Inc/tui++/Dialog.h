#pragma once

#include <tui++/Frame.h>

namespace tui {

class Dialog: public Window {
protected:
  Dialog(const std::shared_ptr<Dialog> &owner);
  Dialog(const std::shared_ptr<Frame> &owner);
  Dialog(const std::shared_ptr<Window> &owner);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

}
