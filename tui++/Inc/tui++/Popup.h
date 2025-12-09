#pragma once

#include <tui++/Window.h>
#include <tui++/ModalExclude.h>

namespace tui {

class Popup {

public:
  class PopupWindow: public Window, public ModalExclude {
    using base = Window;

    PopupWindow(const std::shared_ptr<Window> &owner) :
        base(owner, WindowType::POPUP) {
      set_focusable_window_state(false);
      set_always_on_top(true);
    }

  protected:
    void show() override {
      pack();
      if (get_width() and get_height()) {
        base::show();
      }
    }
  };

};

}
