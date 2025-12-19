#include <tui++/PopupMenu.h>
#include <tui++/Frame.h>

#include <tui++/lookandfeel/PopupMenuUI.h>

namespace tui {

std::shared_ptr<laf::PopupMenuUI> PopupMenu::get_ui() const {
  return std::static_pointer_cast<laf::PopupMenuUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> PopupMenu::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

void PopupMenu::process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {

}

void PopupMenu::process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) {

}

void PopupMenu::menu_selection_changed(bool is_included) {

}

std::vector<std::shared_ptr<MenuElement>> PopupMenu::get_sub_elements() const {
  auto sub_elements = std::vector<std::shared_ptr<MenuElement>> { };
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
