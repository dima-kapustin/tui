#include <tui++/RootPane.h>

namespace tui {

class RootLayout: public Layout {
public:
  virtual Dimension get_preferred_layout_size(const std::shared_ptr<const Component> &target) override {
    auto root_pane = std::static_pointer_cast<const RootPane>(target);
    auto rd = root_pane->content_pane ? root_pane->content_pane->get_preferred_size() : root_pane->get_size();
    auto mbd = root_pane->menu_bar and root_pane->menu_bar->is_visible() ? root_pane->menu_bar->get_preferred_size() : Dimension { };
    auto insets = root_pane->get_insets();
    return {std::max(rd.width, mbd.width) + insets.left + insets.right, rd.height + mbd.height + insets.top + insets.bottom};
  }

  virtual Dimension get_minimum_layout_size(const std::shared_ptr<const Component> &target) override {
    auto root_pane = std::static_pointer_cast<const RootPane>(target);
    auto rd = root_pane->content_pane ? root_pane->content_pane->get_minimum_size() : root_pane->get_size();
    auto mbd = root_pane->menu_bar and root_pane->menu_bar->is_visible() ? root_pane->menu_bar->get_minimum_size() : Dimension { };
    auto insets = root_pane->get_insets();
    return {std::max(rd.width, mbd.width) + insets.left + insets.right, rd.height + mbd.height + insets.top + insets.bottom};
  }

  virtual Dimension get_maximum_layout_size(const std::shared_ptr<const Component> &target) override {
    auto root_pane = std::static_pointer_cast<const RootPane>(target);
    auto mbd = root_pane->menu_bar and root_pane->menu_bar->is_visible() ? root_pane->menu_bar->get_maximum_size() : Dimension { };
    auto insets = root_pane->get_insets();
    auto rd = root_pane->content_pane ? root_pane->content_pane->get_maximum_size() : Dimension { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - insets.top - insets.bottom - mbd.height - 1 };
    return {std::max(rd.width, mbd.width) + insets.left + insets.right, rd.height + mbd.height + insets.top + insets.bottom};
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
      auto mbd = root_pane->menu_bar->get_preferred_size();
      root_pane->menu_bar->set_bounds(0, 0, w, mbd.height);
      content_y += mbd.height;
    }

    if (root_pane->content_pane) {
      root_pane->content_pane->set_bounds(0, content_y, w, h - content_y);
    }
  }

  virtual void add_layout_component(const std::shared_ptr<Component>&, const std::any&) override {
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

void RootPane::set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar) {

}

void RootPane::set_content_pane(const std::shared_ptr<Component> &content_pane) {

}

void RootPane::set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane) {

}

void RootPane::set_glass_pane(const std::shared_ptr<Component> &glass_pane) {

}

void RootPane::set_default_dutton(const std::shared_ptr<Button> &default_dutton) {

}

std::shared_ptr<Component> RootPane::create_class_pane() {

}

std::shared_ptr<LayeredPane> RootPane::create_layered_pane() {
  return std::make_shared<LayeredPane>();
}

std::shared_ptr<Component> RootPane::create_content_pane() {

}

std::shared_ptr<Layout> RootPane::create_root_layout() {
  return std::make_shared<RootLayout>();
}

}
