#include <tui++/PopupMenu.h>

#include <tui++/Menu.h>
#include <tui++/MenuItem.h>
#include <tui++/MenuSelectionManager.h>
#include <tui++/Separator.h>
#include <tui++/Frame.h>
#include <tui++/Popup.h>
#include <tui++/Window.h>

#include <tui++/lookandfeel/PopupMenuUI.h>
#include <tui++/lookandfeel/PopupMenuSeparatorUI.h>

#include <utility>

namespace tui {

class PopupMenuSeparator: public Separator {
public:
  std::shared_ptr<laf::PopupMenuSeparatorUI> get_ui() const {
    return std::static_pointer_cast<laf::PopupMenuSeparatorUI>(this->ui.value());
  }

protected:
  PopupMenuSeparator() :
      Separator(Orientation::HORIZONTAL) {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  std::shared_ptr<laf::ComponentUI> create_ui() override {
    return laf::LookAndFeel::create_ui(this);
  }
};

std::shared_ptr<laf::PopupMenuUI> PopupMenu::get_ui() const {
  return std::static_pointer_cast<laf::PopupMenuUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> PopupMenu::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

std::shared_ptr<MenuItem> PopupMenu::add(std::string const &label) {
  auto menu_item = make_component<MenuItem>(label);
  add(menu_item);
  return menu_item;
}

std::shared_ptr<MenuItem> PopupMenu::add(std::shared_ptr<Action> const &action) {
  auto menu_item = create_item(action);
  add(menu_item);
  return menu_item;
}

void PopupMenu::add_separator() {
  base::add(make_component<PopupMenuSeparator>());
}

void PopupMenu::insert(std::shared_ptr<Component> const &c, size_t index) {
  auto items = std::vector<std::shared_ptr<Component>> {};
  if (index < this->components.size()) {
    items.reserve(this->components.size() - index);
    for (auto i = index; i < this->components.size(); ++i) {
      items.emplace_back(this->components[i]);
      remove(index);
    }
  }

  add(c);

  for (auto &&item : items) {
    add(item);
  }
}

std::shared_ptr<MenuItem> PopupMenu::insert(std::shared_ptr<Action> const &action, size_t index) {
  auto menu_item = create_item(action);
  insert(menu_item, index);
  return menu_item;
}

std::shared_ptr<MenuItem> PopupMenu::create_item(std::shared_ptr<Action> const &action) {
  auto menu_item = make_component<MenuItem>();
  menu_item->set_action(action);
  menu_item->set_horizontal_text_position(HorizontalTextPosition::TRAILING);
  menu_item->set_vertical_text_position(VerticalTextPosition::CENTER);
  return menu_item;
}

void PopupMenu::set_label(std::string const& label) {
  if (this->label != label) {
    this->label = label;
    invalidate();
    repaint();
  }
}

bool PopupMenu::is_popup_menu() const {
  return not this->invoker or not is_a<Menu>(this->invoker);
}

std::shared_ptr<PopupMenu> PopupMenu::get_root_popup_menu() const {
  auto candidate = std::static_pointer_cast<PopupMenu>(const_cast<PopupMenu*>(this)->shared_from_this());
  while (candidate and not candidate->is_popup_menu() and candidate->invoker) {
    if (auto parent = std::dynamic_pointer_cast<PopupMenu>(candidate->invoker->get_parent())) {
      candidate = parent;
    } else {
      break;
    }
  }
  return candidate;
}

void PopupMenu::process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {
  // empty
}

void PopupMenu::process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {
  auto menu_event = MenuKeyEvent {e, path, manager};
  process_event(menu_event);
  if (menu_event.consumed) {
    e.consume();
  }
}

void PopupMenu::menu_selection_changed(bool is_included) {
  if(auto menu = std::dynamic_pointer_cast<Menu>(invoker)) {
    menu->set_popup_menu_visible(is_included);
  } else if (is_popup_menu() and not is_included) {
    set_visible(false);
  }
}

std::vector<std::shared_ptr<MenuElement>> PopupMenu::get_sub_elements() const {
  auto sub_elements = std::vector<std::shared_ptr<MenuElement>> {};
  sub_elements.reserve(this->components.size());
  for (auto &&c : this->components) {
    if (auto candidate = std::dynamic_pointer_cast<MenuElement>(c)) {
      sub_elements.emplace_back(candidate);
    }
  }
  return sub_elements;
}

std::shared_ptr<Component> PopupMenu::get_component() const {
  return const_cast<PopupMenu*>(this)->shared_from_this();
}

void PopupMenu::set_invoker(std::shared_ptr<Component> const &invoker) {
  if (this->invoker != invoker) {
    this->invoker = invoker;
    if (this->ui) {
      this->ui->uninstall_ui(shared_from_this());
      this->ui->install_ui(shared_from_this());
    }
  }

  invalidate();
}

static std::shared_ptr<Frame> get_frame(std::shared_ptr<Component> const &c) {
  for (auto w = c; w; w = w->get_parent()) {
    if (auto frame = std::dynamic_pointer_cast<Frame>(w)) {
      return frame;
    }
  }
  return {};
}

void PopupMenu::show(std::shared_ptr<Component> const &invoker, int x, int y) {
  set_invoker(invoker);

  auto frame = get_frame(invoker);
  if (frame != this->frame) {
    // Use the invoker's frame so that events
    // are propagated properly
    if (frame) {
      this->frame = frame;
      if (this->popup) {
        set_visible(false);
      }
    }
  }

  if (invoker) {
    auto invoker_origin = invoker->get_location_on_screen();
    set_location(invoker_origin.x + x, invoker_origin.y + y);
  } else {
    set_location(x, y);
  }
  set_visible(true);
}

void PopupMenu::set_visible(bool value) {
  if (value) {
    if (is_popup_showing()) {
      return;
    }

    // This is a popup menu with MenuElement children,
    // set selection path before popping up!
    if (is_popup_menu()) {
      auto path = std::vector<std::shared_ptr<MenuElement>> {std::static_pointer_cast<MenuElement>(std::static_pointer_cast<PopupMenu>(shared_from_this()))};
      MenuSelectionManager::single->set_selected_path(path);
    }

    fire_event<PopupMenuEvent>(std::static_pointer_cast<PopupMenu>(shared_from_this()), PopupMenuEvent::BECOMES_VISIBLE);
    show_popup();
  } else {
    // The popup reference, not is_popup_showing(): a popup whose window went
    // off the screen (see Screen::hide_window) still has to be dropped and
    // its take-down run.
    if (not this->popup) {
      return;
    }

    drop_popup();
  }
}

void PopupMenu::set_location(int x, int y) {
  if (this->desired_location.x != x or this->desired_location.y != y) {
    this->desired_location.x = x;
    this->desired_location.y = y;
    if (this->popup) {
      show_popup();
    }
  }
}

void PopupMenu::show_popup() {
  // Let the old popup go before hiding its window: hiding fires the window's
  // hide, and a reference still held here would make the menu take itself
  // down -- a popup that merely moves (set_location re-shows it) must not
  // fire BECOMES_INVISIBLE on the way, and a popup whose window went off the
  // screen behind the menu's back is gone either way.
  if (auto old_popup = std::exchange(this->popup, nullptr)) {
    old_popup->hide();
  }

  this->popup = get_ui()->get_popup(std::static_pointer_cast<PopupMenu>(shared_from_this()), this->desired_location.x, this->desired_location.y);
  this->popup->show();
}

bool PopupMenu::is_popup_showing() const {
  // A popup is only showing while its window is on the screen: the screen
  // takes the popups stacked above a hidden window down with it (see
  // Screen::hide_window), so the menu's reference to the popup can outlive
  // the window's stay on the screen. Asking the window keeps the menu (and
  // whatever opened it) from believing in a dropdown the screen no longer
  // shows.
  return this->popup != nullptr and this->popup->get_window() and this->popup->get_window()->is_showing();
}

void PopupMenu::drop_popup() {
  if (not this->popup) {
    return;
  }

  // Dismiss the submenus hanging off this popup first: their windows sit over
  // this one (and the frame), so closing the parent must take the whole chain
  // down with it -- recursively, for nested submenus.
  for (auto &&child : this->components) {
    if (auto submenu = std::dynamic_pointer_cast<Menu>(child); submenu and submenu->is_popup_menu_visible()) {
      submenu->set_popup_menu_visible(false);
    }
  }

  this->selection_model->clear_selection();

  // The menu is no longer showing as soon as the reference is dropped, so a
  // listener that hides the popup again while the event is delivered (a
  // nested hide) has nothing left to hide; hiding the window is a no-op when
  // the screen already took it down.
  auto popup = std::exchange(this->popup, nullptr);
  fire_event<PopupMenuEvent>(std::static_pointer_cast<PopupMenu>(shared_from_this()), PopupMenuEvent::BECOMES_INVISIBLE);
  popup->hide();

  // A menu that was a component's context menu is a widget's own popup again
  // until the popup trigger makes it one once more (see is_context_menu); a
  // program that hides and re-shows the menu by hand must not keep the menu
  // system's keyboard session alive.
  this->shown_as_context_menu = false;

  // Drop the hover/selection highlight of every item so a popup that is
  // re-shown later starts unarmed (the pointer is no longer on them).
  for (auto &&child : this->components) {
    if (auto item = std::dynamic_pointer_cast<MenuItem>(child); item and item->is_armed()) {
      item->set_armed(false);
    }
  }

  if (is_popup_menu()) {
    MenuSelectionManager::single->clear_selected_path();
  }
}

}
