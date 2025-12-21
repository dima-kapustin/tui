#include <tui++/Frame.h>

#include <tui++/RootPane.h>
#include <tui++/BorderLayout.h>

#include <tui++/lookandfeel/FrameUI.h>

namespace tui {

void Frame::init() {
  Window::init();
  enable_events(KEY_EVENT_MASK | WINDOW_EVENT_MASK);
  this->root_pane->set_window_decoration_style(RootPane::FRAME);
}

std::shared_ptr<laf::FrameUI> Frame::get_ui() const {
  return std::static_pointer_cast<laf::FrameUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> Frame::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

std::shared_ptr<MenuBar> Frame::get_menu_bar() const {
  return this->root_pane->get_menu_bar();
}

void Frame::set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {
  this->root_pane->set_menu_bar(menu_bar);
}

}
