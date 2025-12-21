#pragma once

#include <tui++/Window.h>
#include <tui++/ModalExclude.h>

namespace tui {

class PopupWindow: public Window, public ModalExclude {
  using base = Window;

  PopupWindow(const std::shared_ptr<Window> &owner) :
      base(owner, WindowType::POPUP) {
    set_focusable_window_state(false);
    set_always_on_top(true);
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  void show() override {
    pack();
    if (get_width() and get_height()) {
      base::show();
    }
  }
};

}
