#pragma once

#include <tui++/Window.h>
#include <tui++/ModalExclude.h>

namespace tui {

class PopupWindow: public Window, public ModalExclude {
  using base = Window;

  // What the popup hangs off (a menu row, a combo box) and where its top-left
  // corner sits relative to it. A popup keeps that placement until it is shown
  // again, so a screen-wide relayout can put it back where it belongs (see
  // reanchor).
  std::weak_ptr<Component> anchor;
  Point anchor_offset;

  PopupWindow(const std::shared_ptr<Window> &owner) :
      base(owner, WindowType::POPUP) {
    set_focusable_window_state(false);
    set_always_on_top(true);
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

public:
  // The placement the popup is shown with: `component` is what it hangs off
  // and `offset` where it sits relative to that component's top-left corner
  // (the menu system and the combo box compute both when they open it).
  void set_anchor(std::shared_ptr<Component> const &component, Point const &offset) {
    this->anchor = component;
    this->anchor_offset = offset;
  }

  // Places a showing popup again after a screen-wide relayout (see
  // Screen::revalidate_windows): a translation resizes the popup's items and
  // can move the component it hangs off, so the popup is packed to its
  // contents and moved to its anchor's new position. A popup whose anchor
  // sits in another popup (a submenu hangs off a row of its parent menu) is
  // placed after that one, whose own new size lays the row out.
  void reanchor();

protected:
  // A popup created for its own sake (a tooltip, an editor's completer)
  // takes the generic popup shadow; the popup a menu, a combo box or a
  // dialog opens carries its own key instead (see Popup).
  virtual std::string_view get_shadow_key() const override {
    return "PopupWindow.Shadow";
  }

  void show() override {
    pack();
    if (get_width() and get_height()) {
      base::show();
    }
  }
};

}
