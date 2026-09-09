#include <tui++/MenuKeyboardManager.h>

#include <tui++/CheckBoxMenuItem.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/MenuSelectionManager.h>
#include <tui++/PopupMenu.h>
#include <tui++/RadioButtonMenuItem.h>
#include <tui++/Window.h>

#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/lookandfeel/LookAndFeel.h>

#include <algorithm>
#include <chrono>
#include <optional>

namespace tui {
namespace {

// ASCII case folding for mnemonic matching: 'f' selects "File" and 'F'
// selects "find" just the same.
constexpr char32_t fold_case(char32_t code) {
  return code >= 'A' and code <= 'Z' ? code - 'A' + 'a' : code;
}

// The printable character of a key event, when it has one. Letters, digits
// and punctuation arrive as KEY_TYPED characters; KEY_PRESSED carries their
// code points for the few hosts that send them that way.
std::optional<char32_t> key_char(KeyEvent const &e) {
  auto code = e.id == KeyEvent::KEY_TYPED ? e.get_key_char().get_code() : char32_t(e.get_key_code());
  if (code == 0x20 or (code > 0x20 and code < 0x7f)) {
    return code;
  }
  return std::nullopt;
}

bool has_ctrl(KeyEvent const &e) {
  return bool(e.modifiers & InputEvent::CTRL_DOWN);
}

bool has_alt(KeyEvent const &e) {
  return bool(e.modifiers & (InputEvent::ALT_DOWN | InputEvent::META_DOWN));
}

// Whether the event is the space key, however the host delivered it (a
// KEY_PRESSED VK_SPACE or a KEY_TYPED space character).
bool is_space(KeyEvent const &e) {
  if (e.id == KeyEvent::KEY_PRESSED and e.get_key_code() == KeyEvent::VK_SPACE) {
    return true;
  }
  return e.id == KeyEvent::KEY_TYPED and e.get_key_char().get_code() == 0x20;
}

// Whether the event is the Escape key (KEY_PRESSED VK_ESCAPE, or the odd
// host that delivers ESC as a typed control character).
bool is_escape(KeyEvent const &e) {
  if (e.id == KeyEvent::KEY_PRESSED and e.get_key_code() == KeyEvent::VK_ESCAPE) {
    return true;
  }
  return e.id == KeyEvent::KEY_TYPED and e.get_key_char().get_code() == 0x1b;
}

// The menu bar of the window a key/mouse event was dispatched to. Only
// frames carry menu bars; popup windows and dialogs do not, so keys aimed at
// them never touch the menu system of the frame underneath.
std::shared_ptr<MenuBar> menu_bar_of(const std::shared_ptr<Window> &window) {
  if (auto frame = std::dynamic_pointer_cast<Frame>(window)) {
    return frame->get_menu_bar();
  }
  return {};
}

// The top-level menus of the bar, in bar order (Swing's JMenuBar children).
std::vector<std::shared_ptr<Menu>> top_level_menus(MenuBar const &bar) {
  auto menus = std::vector<std::shared_ptr<Menu>> { };
  menus.reserve(bar.get_component_count());
  for (auto &&component : bar.get_components()) {
    if (auto menu = std::dynamic_pointer_cast<Menu>(component)) {
      menus.emplace_back(std::move(menu));
    }
  }
  return menus;
}

std::shared_ptr<Menu> first_enabled(std::vector<std::shared_ptr<Menu>> const &menus) {
  for (auto &&menu : menus) {
    if (menu->is_enabled()) {
      return menu;
    }
  }
  return {};
}

// The menu whose popup is currently on the screen, or null.
std::shared_ptr<Menu> open_popup_menu(MenuBar const &bar) {
  for (auto &&menu : top_level_menus(bar)) {
    if (menu->is_popup_menu_visible()) {
      return menu;
    }
  }
  return {};
}

// The menu items of an open popup that the keyboard can select. Separators
// are not menu items and drop out of the list by themselves, as do the
// disabled items the navigation has to skip.
std::vector<std::shared_ptr<MenuItem>> popup_items(PopupMenu const &popup) {
  auto items = std::vector<std::shared_ptr<MenuItem>> { };
  items.reserve(popup.get_component_count());
  for (auto &&component : popup.get_components()) {
    if (auto item = std::dynamic_pointer_cast<MenuItem>(component); item and item->is_enabled()) {
      items.emplace_back(std::move(item));
    }
  }
  return items;
}

// The armed item of an open popup (the hovered item when the mouse is in the
// popup, the keyboard selection otherwise), or null.
std::shared_ptr<MenuItem> armed_popup_item(PopupMenu const &popup) {
  for (auto &&component : popup.get_components()) {
    if (auto item = std::dynamic_pointer_cast<MenuItem>(component); item and item->is_armed() and item->is_enabled()) {
      return item;
    }
  }
  return {};
}

// Arms `menu` and un-arms every sibling top-level menu, so the keyboard
// highlight never shows on two menus of the bar at once.
void arm_top_level_menu(MenuBar const &bar, Menu const *menu) {
  for (auto &&sibling : top_level_menus(bar)) {
    if (sibling->is_armed() != (sibling.get() == menu)) {
      sibling->set_armed(sibling.get() == menu);
      sibling->repaint();
    }
  }
}

void unarm_top_level_menus(MenuBar const &bar) {
  for (auto &&menu : top_level_menus(bar)) {
    if (menu->is_armed()) {
      menu->set_armed(false);
      menu->repaint();
    }
  }
}

// Puts `item` of `popup` on the selection path: menu_selection_changed arms
// the item and un-arms whatever item was selected before, exactly as
// hovering an item does with the mouse.
void select_popup_item(std::shared_ptr<PopupMenu> const &popup, std::shared_ptr<MenuItem> const &item) {
  auto path = std::vector<std::shared_ptr<MenuElement>> {
    std::static_pointer_cast<MenuElement>(popup), std::static_pointer_cast<MenuElement>(item)
  };
  MenuSelectionManager::single->set_selected_path(path);

  // The path change arms the item it ends at (menu_selection_changed), but a
  // stale path can already end at `item` while the item is un-armed (the
  // mouse dropped the highlight of a keyboard selection without leaving the
  // path). Arm explicitly so a selection step always shows where it moved;
  // no-op when the path change already armed the item.
  if (not item->is_armed()) {
    item->set_armed(true);
    item->repaint();
  }
}

// Moves the popup selection one item up (-1) or down (+1), skipping disabled
// items, with wrap-around at the ends (native Windows menus wrap; so do we).
// With nothing armed yet the first press picks an end: down reaches the
// first item, up the last.
void move_popup_selection(std::shared_ptr<PopupMenu> const &popup, int direction) {
  auto items = popup_items(*popup);
  if (items.empty()) {
    return;
  }

  auto current = size_t { 0 };
  auto found = false;
  if (auto armed = armed_popup_item(*popup)) {
    for (auto i = size_t { 0 }, n = items.size(); i != n; ++i) {
      if (items[i] == armed) {
        current = i;
        found = true;
        break;
      }
    }
  }

  size_t next;
  if (not found) {
    next = direction > 0 ? 0 : items.size() - 1;
  } else {
    next = (current + (direction > 0 ? 1 : items.size() - 1)) % items.size();
  }
  select_popup_item(popup, items[next]);
}

// Activates `item` exactly like a mouse release on it (MenuItemUI::do_click):
// the selection path is cleared first, which takes the popup off the path so
// its menu_selection_changed(false) hides the popup window, then the item is
// clicked (its ActionEvent listeners run -- in the demos they perform the
// edit and dismiss the popup again, which is now a no-op).
void click_popup_item(std::shared_ptr<Menu> const &open, std::shared_ptr<MenuItem> const &item) {
  auto do_not_close = false;
  if (is_a<CheckBoxMenuItem>(item)) {
    do_not_close = laf::LookAndFeel::get<bool>(item.get(), "CheckBoxMenuItem.DoNotCloseOnMouseClick");
  } else if (is_a<RadioButtonMenuItem>(item)) {
    do_not_close = laf::LookAndFeel::get<bool>(item.get(), "RadioButtonMenuItem.DoNotCloseOnMouseClick");
  }
  if (not do_not_close) {
    MenuSelectionManager::single->clear_selected_path();
  }
  item->do_click(std::chrono::milliseconds::zero());
  if (open->is_popup_menu_visible()) {
    open->set_popup_menu_visible(false);
  }
}

// The armed, enabled top-level menu of the bar, or null.
std::shared_ptr<Menu> armed_top_level_menu(MenuBar const &bar) {
  for (auto &&menu : top_level_menus(bar)) {
    if (menu->is_armed() and menu->is_enabled()) {
      return menu;
    }
  }
  return {};
}

// The top-level menu whose mnemonic is `code`, or null.
std::shared_ptr<Menu> menu_with_mnemonic(std::vector<std::shared_ptr<Menu>> const &menus, char32_t code) {
  auto wanted = fold_case(code);
  for (auto &&menu : menus) {
    if (menu->is_enabled() and fold_case(menu->get_mnemonic().get_code()) == wanted) {
      return menu;
    }
  }
  return {};
}

// The item of `popup` whose mnemonic is `code`, or null.
std::shared_ptr<MenuItem> item_with_mnemonic(PopupMenu const &popup, char32_t code) {
  auto wanted = fold_case(code);
  for (auto &&item : popup_items(popup)) {
    if (fold_case(item->get_mnemonic().get_code()) == wanted) {
      return item;
    }
  }
  return {};
}

// Closes `open`'s popup and opens `target`'s in its place, arming the bar
// the way a mouse hover that switches the open popup does.
void switch_open_popup(std::shared_ptr<Menu> const &open, std::shared_ptr<Menu> const &target, MenuBar const &bar) {
  if (open == target) {
    return;
  }
  open->set_popup_menu_visible(false);
  arm_top_level_menu(bar, target.get());
  target->set_popup_menu_visible(true);
}

} // namespace

bool MenuKeyboardManager::handle_key_event(const std::shared_ptr<Window> &window, KeyEvent &e) {
  if (e.id != KeyEvent::KEY_PRESSED and e.id != KeyEvent::KEY_TYPED) {
    return false;
  }
  if (auto bar = menu_bar_of(window); bar and bar->is_showing() and bar->is_enabled()) {
    return handle_key_event(bar, e);
  }
  return false;
}

bool MenuKeyboardManager::handle_key_event(const std::shared_ptr<MenuBar> &bar, KeyEvent &e) {
  if (auto open = open_popup_menu(*bar)) {
    return handle_open_popup_key(open, e);
  }
  if (this->keyboard_mode_bars.contains(bar.get())) {
    return handle_menu_mode_key(bar, e);
  }
  return handle_idle_key(bar, e);
}

bool MenuKeyboardManager::handle_open_popup_key(const std::shared_ptr<Menu> &open, KeyEvent &e) {
  auto const bar = std::dynamic_pointer_cast<MenuBar>(open->get_parent());
  if (not bar) {
    return false; // A submenu's popup has no keyboard navigation yet
  }

  if (not has_ctrl(e) and is_escape(e)) {
    // Escape closes the popup and leaves the keyboard on the bar (a second
    // Escape leaves the bar again); Windows and Swing behave the same.
    open->set_popup_menu_visible(false);
    arm_top_level_menu(*bar, open.get());
    this->keyboard_mode_bars.insert(bar.get());
    e.consume();
    return true;
  }

  if (e.id == KeyEvent::KEY_PRESSED) {
    auto menus = top_level_menus(*bar);
    auto open_index = std::find(menus.begin(), menus.end(), open) - menus.begin();

    switch (e.get_key_code()) {
    case KeyEvent::VK_UP:
    case KeyEvent::VK_DOWN:
      // Move through the popup's items; the popup holds the keyboard until
      // it closes, so even modifier chords (the text area's Ctrl+arrow word
      // moves, Alt+arrow column gestures) stay with the menu.
      move_popup_selection(open->get_popup_menu(), e.get_key_code() == KeyEvent::VK_DOWN ? 1 : -1);
      e.consume();
      return true;

    case KeyEvent::VK_LEFT:
    case KeyEvent::VK_RIGHT: {
      // Left/right hop between the top-level menus' popups, like a hover
      // that glides over the bar. At the ends the popup of the first menu
      // closes back onto the bar; the last menu's popup simply stays.
      auto target = std::shared_ptr<Menu> { };
      if (e.get_key_code() == KeyEvent::VK_RIGHT) {
        for (auto i = open_index + 1; i < (int) menus.size(); ++i) {
          if (menus[i]->is_enabled()) {
            target = menus[i];
            break;
          }
        }
      } else {
        for (auto i = open_index - 1; i >= 0; --i) {
          if (menus[i]->is_enabled()) {
            target = menus[i];
            break;
          }
        }
      }

      if (target) {
        switch_open_popup(open, target, *bar);
      } else if (e.get_key_code() == KeyEvent::VK_LEFT and open_index == 0) {
        open->set_popup_menu_visible(false);
        arm_top_level_menu(*bar, open.get());
        this->keyboard_mode_bars.insert(bar.get());
      }
      e.consume();
      return true;
    }

    case KeyEvent::VK_ENTER:
      this->activate_armed_popup_item(open);
      e.consume();
      return true;

    case KeyEvent::VK_F10:
      if (not has_ctrl(e)) {
        open->set_popup_menu_visible(false);
        this->keyboard_mode_bars.erase(bar.get());
        unarm_top_level_menus(*bar);
        e.consume();
        return true;
      }
      return false;

    case KeyEvent::VK_BACK_SPACE:
    case KeyEvent::VK_DELETE:
    case KeyEvent::VK_HOME:
    case KeyEvent::VK_END:
    case KeyEvent::VK_PAGE_UP:
    case KeyEvent::VK_PAGE_DOWN:
    case KeyEvent::VK_INSERT:
      // Editing keys would hit the text behind the open popup; the open menu
      // is modal for them.
      e.consume();
      return true;

    default:
      break;
    }
  }

  // Space activates the armed item like Enter, however the host delivered
  // it (some send VK_SPACE as a key press, others as a typed character).
  if (is_space(e) and not has_ctrl(e)) {
    this->activate_armed_popup_item(open);
    e.consume();
    return true;
  }

  // Mnemonic letters: with an open popup a plain letter selects the item it
  // stands for ("P" picks Paste); Alt+letter moves to the popup of the
  // top-level menu it stands for (Alt+F returns to File). Letters no menu
  // item claims are swallowed: the popup is modal and must not type into the
  // component underneath. Ctrl chords pass through so the item accelerators
  // (Ctrl+Z, ...) keep working with a menu open.
  if (not has_ctrl(e)) {
    if (auto ch = key_char(e)) {
      if (has_alt(e)) {
        if (auto target = menu_with_mnemonic(top_level_menus(*bar), *ch)) {
          switch_open_popup(open, target, *bar);
        }
      } else if (auto item = item_with_mnemonic(*open->get_popup_menu(), *ch)) {
        select_popup_item(open->get_popup_menu(), item);
      }
      e.consume();
      return true;
    }
  }
  return false;
}

bool MenuKeyboardManager::handle_menu_mode_key(const std::shared_ptr<MenuBar> &bar, KeyEvent &e) {
  if (not has_ctrl(e) and is_escape(e)) {
    // Escape (and F10 again) leave the menu bar: the armed highlight goes
    // and the keys belong to the component underneath again.
    this->keyboard_mode_bars.erase(bar.get());
    unarm_top_level_menus(*bar);
    e.consume();
    return true;
  }

  if (e.id == KeyEvent::KEY_PRESSED) {
    auto menus = top_level_menus(*bar);
    auto enabled = std::vector<std::shared_ptr<Menu>> { };
    for (auto &&menu : menus) {
      if (menu->is_enabled()) {
        enabled.emplace_back(menu);
      }
    }

    auto armed = std::shared_ptr<Menu> { };
    for (auto &&menu : enabled) {
      if (menu->is_armed()) {
        armed = menu;
        break;
      }
    }

    switch (e.get_key_code()) {
    case KeyEvent::VK_LEFT:
    case KeyEvent::VK_RIGHT: {
      // Move the armed highlight across the top-level menus (no wrap, like
      // the mouse: gliding past the end of the bar stops there).
      auto next = std::shared_ptr<Menu> { };
      if (enabled.size() > 1) {
        auto at = std::find(enabled.begin(), enabled.end(), armed);
        if (e.get_key_code() == KeyEvent::VK_RIGHT) {
          if (at != enabled.end() and at + 1 != enabled.end()) {
            next = *(at + 1);
          }
        } else if (at != enabled.begin()) {
          next = *(at - 1);
        } else if (at == enabled.end()) {
          next = enabled.front();
        }
      }
      if (next) {
        arm_top_level_menu(*bar, next.get());
      }
      e.consume();
      return true;
    }

    case KeyEvent::VK_DOWN:
    case KeyEvent::VK_UP:
    case KeyEvent::VK_ENTER:
      if (auto armed = armed_top_level_menu(*bar)) {
        if (e.get_key_code() == KeyEvent::VK_DOWN or e.get_key_code() == KeyEvent::VK_UP) {
          // Down/up walk into the popup, so they arm its first/last item;
          // Enter only opens it (the first arrow picks an item).
          this->open_popup_with_selection(armed, e.get_key_code() == KeyEvent::VK_DOWN ? 1 : -1);
        } else {
          this->open_popup_with_selection(armed, 0);
        }
      }
      e.consume();
      return true;

    case KeyEvent::VK_F10:
      if (not has_ctrl(e)) {
        this->keyboard_mode_bars.erase(bar.get());
        unarm_top_level_menus(*bar);
        e.consume();
        return true;
      }
      return false;

    default:
      break;
    }
  }

  // Space opens the armed menu's popup, however it was delivered.
  if (is_space(e) and not has_ctrl(e)) {
    if (auto armed = armed_top_level_menu(*bar)) {
      this->open_popup_with_selection(armed, 0);
    }
    e.consume();
    return true;
  }

  // Mnemonic letters while the bar is armed: Alt+letter and plain letters
  // alike open the menu they stand for. Like an open popup, the armed bar
  // swallows the letters no menu claims (they would otherwise type into the
  // component while the bar visibly holds the keyboard).
  if (not has_ctrl(e)) {
    if (auto ch = key_char(e)) {
      if (auto target = menu_with_mnemonic(top_level_menus(*bar), *ch)) {
        arm_top_level_menu(*bar, target.get());
        target->set_popup_menu_visible(true);
      }
      e.consume();
      return true;
    }
  }
  return false;
}

bool MenuKeyboardManager::handle_idle_key(const std::shared_ptr<MenuBar> &bar, KeyEvent &e) {
  if (e.id == KeyEvent::KEY_PRESSED and not has_ctrl(e) and e.get_key_code() == KeyEvent::VK_F10) {
    // F10 puts the menu bar on the keyboard, highlighting its first menu
    // (a second F10, or Escape, leaves again). With no enabled menu there
    // is nothing to arm and the key falls through.
    if (auto first = first_enabled(top_level_menus(*bar))) {
      arm_top_level_menu(*bar, first.get());
      this->keyboard_mode_bars.insert(bar.get());
      e.consume();
      return true;
    }
    return false;
  }

  // Alt+mnemonic (Alt+F opens File) arms the menu and opens its popup, as on
  // Windows; the bar stays armed underneath so left/right after Escape hops
  // between the menus.
  if (not has_ctrl(e) and has_alt(e)) {
    if (auto ch = key_char(e)) {
      if (auto target = menu_with_mnemonic(top_level_menus(*bar), *ch)) {
        arm_top_level_menu(*bar, target.get());
        this->keyboard_mode_bars.insert(bar.get());
        target->set_popup_menu_visible(true);
        e.consume();
        return true;
      }
    }
  }
  return false;
}

void MenuKeyboardManager::activate_armed_popup_item(const std::shared_ptr<Menu> &open) {
  auto item = armed_popup_item(*open->get_popup_menu());
  if (not item) {
    return; // Nothing is armed; the popup stays open and the session alive
  }

  click_popup_item(open, item);

  // The pick dismissed the popup (through the selection path, or by the
  // item's own action listener); the keyboard session is over and the armed
  // highlight of the bar goes off, as it does after a mouse pick.
  if (auto bar = std::dynamic_pointer_cast<MenuBar>(open->get_parent())) {
    this->keyboard_mode_bars.erase(bar.get());
    unarm_top_level_menus(*bar);
  }
}

void MenuKeyboardManager::open_popup_with_selection(const std::shared_ptr<Menu> &armed, int direction) {
  armed->set_popup_menu_visible(true);
  if (direction != 0) {
    // The arrow that walked into the popup arms its first (down) or last
    // (up) enabled item, so the next arrow keeps moving from there.
    auto popup = armed->get_popup_menu();
    auto items = popup_items(*popup);
    if (not items.empty()) {
      select_popup_item(popup, direction > 0 ? items.front() : items.back());
    }
  }
}

void MenuKeyboardManager::handle_mouse_pressed(const std::shared_ptr<Window> &window, MousePressEvent const &e) {
  // The mouse owns the menus from its first press: drop every keyboard
  // session (an open popup still captures its keys, but only while it stays
  // open -- closing it with the mouse does not bring the F10 mode back).
  this->keyboard_mode_bars.clear();

  // A press outside the menu bar drops the armed highlight the keyboard put
  // there (the pointer is about to take over the bar, or is elsewhere
  // entirely and the highlight would linger without a mouse on it).
  if (auto bar = menu_bar_of(window)) {
    auto point = convert_point_to_screen(e.point, window);
    auto origin = bar->get_location_on_screen();
    auto size = bar->get_size();
    if (point.x < origin.x or point.x >= origin.x + size.width or point.y < origin.y or point.y >= origin.y + size.height) {
      unarm_top_level_menus(*bar);
    }
  }
}

}
