#pragma once

#include <memory>
#include <unordered_set>

namespace tui {

class MenuBar;
class Menu;
class Window;
class KeyEvent;
class MousePressEvent;

// Keyboard navigation for a window's menu bar, in the spirit of Swing's menu
// keyboard handling and of the native Windows menus: F10 (or Alt+mnemonic)
// puts the menu bar on the keyboard, the arrow keys move the highlight (across
// the top-level menus, then through the items of the open popup), Enter/Space
// activates the armed item, Escape steps back out, and the mnemonic letters
// select their items. Window::dispatch_event gives this manager the first
// crack at every key so that an open popup captures the navigation keys
// before the focused component (the text area of the demos) could use them.
//
// The manager drives the same state the mouse uses -- the armed highlight and
// the popup windows -- so mouse and keyboard navigation never disagree about
// what is open or selected.
class MenuKeyboardManager {
  // The menu bars in "menu mode": a top-level menu is armed (F10-style) but
  // no popup is open. With a popup open the keyboard session is implied by
  // the popup itself; the set only tracks the popup-less half of the state.
  std::unordered_set<MenuBar*> keyboard_mode_bars;

public:
  static inline std::shared_ptr<MenuKeyboardManager> single = std::make_shared<MenuKeyboardManager>();

  // Handles a key event dispatched to `window` (the window the event was
  // posted to, usually the focused frame). Returns true when the key was
  // consumed by the menu system; Window::dispatch_event then skips the rest
  // of the dispatch (listeners, key bindings, the focused component).
  bool handle_key_event(const std::shared_ptr<Window> &window, KeyEvent &e);

  // A mouse press ends any keyboard menu session: the pointer now drives the
  // menus (click-to-open, hover). A press outside the menu bar also drops the
  // armed highlight of its top-level menus, so the F10 highlight does not
  // linger once the mouse takes over.
  void handle_mouse_pressed(const std::shared_ptr<Window> &window, MousePressEvent const &e);

private:
  // Ends the keyboard session of `bar`: any open popup closes, the armed
  // highlight of the top-level menus goes off.
  void cancel_keyboard_session(const std::shared_ptr<MenuBar> &bar);

  bool handle_key_event(const std::shared_ptr<MenuBar> &bar, KeyEvent &e);
  bool handle_open_popup_key(const std::shared_ptr<Menu> &open, KeyEvent &e);
  bool handle_menu_mode_key(const std::shared_ptr<MenuBar> &bar, KeyEvent &e);
  bool handle_idle_key(const std::shared_ptr<MenuBar> &bar, KeyEvent &e);

  // Clicks the armed item of `open`'s popup (if any) and ends the keyboard
  // session: the popup is dismissed through the selection path, the menu bar
  // highlight goes off.
  void activate_armed_popup_item(const std::shared_ptr<Menu> &open);

  // Opens the popup of the armed top-level menu `armed`; with a nonzero
  // `direction` the first (+1) or last (-1) enabled item is armed right
  // away, as when the user walks into the popup with an arrow key.
  void open_popup_with_selection(const std::shared_ptr<Menu> &armed, int direction);
};

}
