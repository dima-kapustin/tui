#include <tui++/PopupMenu.h>
#include <tui++/MenuItem.h>
#include <tui++/Separator.h>
#include <tui++/Menu.h>
#include <tui++/Frame.h>

#include <tui++/lookandfeel/PopupMenuUI.h>
#include <tui++/lookandfeel/PopupMenuSeparatorUI.h>

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

std::shared_ptr<MenuItem> PopupMenu::add(std::shared_ptr<MenuItem> const &menu_item) {
  base::add(menu_item);
  return menu_item;
}

void PopupMenu::add_separator() {
  base::add(make_component<PopupMenuSeparator>());
}

void PopupMenu::insert(std::shared_ptr<Component> const &c, size_t index) {

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
  return not this->invoker or (dynamic_cast<Menu*>(this->invoker.get()) == nullptr);
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

}

void PopupMenu::process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {

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

}
