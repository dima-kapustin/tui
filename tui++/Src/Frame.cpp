#include <tui++/Frame.h>

#include <tui++/RootPane.h>
#include <tui++/BorderLayout.h>

#include <tui++/lookandfeel/FrameUI.h>

namespace tui {

void Frame::init() {
  Window::init();
  enable_events(KEY_EVENT_MASK | WINDOW_EVENT_MASK);
  set_root_pane(create_root_pane());
  this->root_pane->set_window_decoration_style(RootPane::FRAME);
}

std::shared_ptr<laf::FrameUI> Frame::get_ui() const {
  return std::static_pointer_cast<laf::FrameUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> Frame::create_ui() {
  return laf::LookAndFeel::get_ui(this);
}

void Frame::add_impl(const std::shared_ptr<Component> &c, const std::any &constraints, int z_order) noexcept (false) {
  if (c == this->root_pane) {
    base::add_impl(c, constraints, z_order);
  } else {
    this->root_pane->get_content_pane()->add(c, constraints, z_order);
  }
}

void Frame::remove(const std::shared_ptr<Component> &c) {
  if (c == this->root_pane) {
    base::remove(c);
  } else {
    this->root_pane->get_content_pane()->remove(c);
  }
}

void Frame::set_layout(const std::shared_ptr<Layout> &layout) {
  if (this->root_pane) {
    this->root_pane->get_content_pane()->set_layout(layout);
  } else {
    base::set_layout(layout);
  }
}

std::shared_ptr<RootPane> Frame::create_root_pane() const {
  auto root_pane = std::make_shared<RootPane>();
  root_pane->set_opaque(true);
  return root_pane;
}

std::shared_ptr<MenuBar> Frame::get_menu_bar() const {
  return this->root_pane->get_menu_bar();
}

void Frame::set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {
  this->root_pane->set_menu_bar(menu_bar);
}

std::shared_ptr<RootPane> Frame::get_root_pane() const {
  return this->root_pane;
}

std::shared_ptr<Component> Frame::get_content_pane() const {
  return this->root_pane->get_content_pane();
}

void Frame::set_content_pane(const std::shared_ptr<Component> &content_pane) {
  this->root_pane->set_content_pane(content_pane);
}

std::shared_ptr<LayeredPane> Frame::get_layered_pane() const {
  return this->root_pane->get_layered_pane();
}

void Frame::set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane) {
  this->root_pane->set_layered_pane(layered_pane);
}

std::shared_ptr<Component> Frame::get_glass_pane() const {
  return this->root_pane->get_glass_pane();
}

void Frame::set_glass_pane(const std::shared_ptr<Component> &glass_pane) {
  this->root_pane->set_glass_pane(glass_pane);
}

void Frame::set_root_pane(const std::shared_ptr<RootPane> &root_pane) {
  if (this->root_pane) {
    remove(this->root_pane);
  }

  this->root_pane = root_pane;
  if (this->root_pane) {
    add(this->root_pane, BorderLayout::CENTER);
  }
}

}
