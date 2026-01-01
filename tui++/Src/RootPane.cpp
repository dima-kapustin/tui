#include <tui++/RootPane.h>

#include <tui++/Panel.h>
#include <tui++/Button.h>
#include <tui++/MenuBar.h>
#include <tui++/LayeredPane.h>
#include <tui++/BorderLayout.h>

#include <tui++/lookandfeel/RootPaneUI.h>

namespace tui {

class RootLayout: public Layout {
public:
  virtual std::optional<Dimension> get_preferred_layout_size(const std::shared_ptr<const Component> &target) override {
    auto root_pane = std::static_pointer_cast<const RootPane>(target);
    auto size = root_pane->content_pane ? root_pane->content_pane->get_preferred_size() : root_pane->get_size();
    auto menu_bar_size = root_pane->menu_bar and root_pane->menu_bar->is_visible() ? root_pane->menu_bar->get_preferred_size() : Dimension::zero();
    auto insets = root_pane->get_insets();
    return Dimension { std::max(size.width, menu_bar_size.width) + insets.left + insets.right, size.height + menu_bar_size.height + insets.top + insets.bottom };
  }

  virtual std::optional<Dimension> get_minimum_layout_size(const std::shared_ptr<const Component> &target) override {
    auto root_pane = std::static_pointer_cast<const RootPane>(target);
    auto size = root_pane->content_pane ? root_pane->content_pane->get_minimum_size() : root_pane->get_size();
    auto menu_bar_size = root_pane->menu_bar and root_pane->menu_bar->is_visible() ? root_pane->menu_bar->get_minimum_size() : Dimension::zero();
    auto insets = root_pane->get_insets();
    return Dimension { std::max(size.width, menu_bar_size.width) + insets.left + insets.right, size.height + menu_bar_size.height + insets.top + insets.bottom };
  }

  virtual std::optional<Dimension> get_maximum_layout_size(const std::shared_ptr<const Component> &target) override {
    auto root_pane = std::static_pointer_cast<const RootPane>(target);
    auto menu_bar_size = root_pane->menu_bar and root_pane->menu_bar->is_visible() ? root_pane->menu_bar->get_maximum_size() : Dimension::zero();
    auto insets = root_pane->get_insets();
    auto size = root_pane->content_pane ? root_pane->content_pane->get_maximum_size() : Dimension { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - insets.top - insets.bottom - menu_bar_size.height - 1 };
    return Dimension { std::max(size.width, menu_bar_size.width) + insets.left + insets.right, size.height + menu_bar_size.height + insets.top + insets.bottom };
  }

  virtual void layout(const std::shared_ptr<Component> &target) override {
    auto root_pane = std::static_pointer_cast<RootPane>(target);
    auto bounds = root_pane->get_bounds();
    auto insets = root_pane->get_insets();
    auto content_y = 0;

    auto w = bounds.width - insets.right - insets.left;
    auto h = bounds.height - insets.top - insets.bottom;

    if (root_pane->layered_pane) {
      root_pane->layered_pane->set_bounds(insets.left, insets.top, w, h);
    }

    if (root_pane->glass_pane) {
      root_pane->glass_pane->set_bounds(insets.left, insets.top, w, h);
    }

    // Note: This is laying out the children in the layeredPane,
    // technically, these are not our children.
    if (root_pane->menu_bar and root_pane->menu_bar->is_visible()) {
      auto menu_bar_size = root_pane->menu_bar->get_preferred_size();
      root_pane->menu_bar->set_bounds(0, 0, w, menu_bar_size.height);
      content_y += menu_bar_size.height;
    }

    if (root_pane->content_pane) {
      root_pane->content_pane->set_bounds(0, content_y, w, h - content_y);
    }
  }

  virtual void add_layout_component(const std::shared_ptr<Component>&, const Constraints&) override {
  }

  virtual void remove_layout_component(const std::shared_ptr<Component>&) override {
  }

  virtual float get_layout_alignment_x(const std::shared_ptr<const Component> &target) const override {
    return 0.0f;
  }

  virtual float get_layout_alignment_y(const std::shared_ptr<const Component> &target) const override {
    return 0.0f;
  }

  virtual void invalidate_layout(const std::shared_ptr<const Component> &target) override {
  }
};

void RootPane::init() {
  set_glass_pane(create_class_pane());
  set_layered_pane(create_layered_pane());
  set_content_pane(create_content_pane());
  set_layout(create_root_layout());
}

void RootPane::add_notify() {
  base::add_notify();
  enable_events(KEY_EVENT_MASK);
}

void RootPane::set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {
  if (this->menu_bar and this->menu_bar->get_parent() == this->layered_pane) {
    this->layered_pane->remove(this->menu_bar);
  }

  this->menu_bar = menu_bar;

  if (this->menu_bar) {
    menu_bar->update_ui();
    this->layered_pane->add(this->menu_bar, LayeredPane::FRAME_CONTENT_LAYER);
  }
}

void RootPane::set_content_pane(const std::shared_ptr<Component> &content_pane) {
  if (this->content_pane and this->content_pane->get_parent() == this->layered_pane) {
    this->layered_pane->remove(this->content_pane);
  }
  this->content_pane = content_pane;
  this->layered_pane->add(this->content_pane, LayeredPane::FRAME_CONTENT_LAYER);
}

void RootPane::set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane) {
  if (this->layered_pane and this->layered_pane->get_parent().get() == this) {
    remove(this->layered_pane);
  }
  this->layered_pane = layered_pane;
  add(this->layered_pane, -1);
}

void RootPane::set_glass_pane(const std::shared_ptr<Component> &glass_pane) {
  auto visible = false;
  if (this->glass_pane and this->glass_pane->get_parent().get() == this) {
    visible = this->glass_pane->is_visible();
    remove(this->glass_pane);
  }

  glass_pane->set_visible(visible);
  this->glass_pane = glass_pane;
  add(glass_pane, 0);
  if (visible) {
    repaint();
  }
}

void RootPane::set_default_dutton(const std::shared_ptr<Button> &button) {
  auto old_default_button = this->default_button.value();
  if (old_default_button != button) {
    this->default_button = button;
    if (old_default_button) {
      old_default_button->repaint();
    }
    if (this->default_button) {
      this->default_button->repaint();
    }
  }
}

std::shared_ptr<Component> RootPane::create_class_pane() const {
  auto glass_pane = make_component<Panel>();
  glass_pane->set_visible(false);
  glass_pane->set_opaque(false);
  return glass_pane;
}

std::shared_ptr<LayeredPane> RootPane::create_layered_pane() const {
  return make_component<LayeredPane>();
}

std::shared_ptr<Component> RootPane::create_content_pane() const {
  return make_component < Panel > (std::make_shared<BorderLayout>());
}

std::shared_ptr<Layout> RootPane::create_root_layout() const {
  return std::make_shared<RootLayout>();
}

std::shared_ptr<laf::RootPaneUI> RootPane::get_ui() const {
  return std::static_pointer_cast<laf::RootPaneUI>(this->ui.value());
}

std::shared_ptr<laf::ComponentUI> RootPane::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

std::shared_ptr<RootPane> get_root_pane(const std::shared_ptr<Component> &c) {
  if (auto root_pane_conainter = std::dynamic_pointer_cast<RootPaneContainer>(c)) {
    return root_pane_conainter->get_root_pane();
  }

  for (auto parent = c; parent; parent = parent->get_parent()) {
    if (auto root_pane = std::dynamic_pointer_cast<RootPane>(parent)) {
      return root_pane;
    }
  }
  return nullptr;
}

}
