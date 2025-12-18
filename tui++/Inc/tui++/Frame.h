#pragma once

#include <tui++/Window.h>
#include <tui++/RootPane.h>

namespace tui {
namespace laf {
class FrameUI;
}

class MenuBar;

class Frame: public Window, public RootPaneContainer {
  using base = Window;

  std::shared_ptr<RootPane> root_pane;

  Frame(Screen &screen) :
      Window(screen) {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  Property<std::string> title { this, "title" };

protected:
  void init() override;
  void add_impl(const std::shared_ptr<Component> &c, const std::any &constraints, int z_order) noexcept (false) override;

  virtual std::shared_ptr<RootPane> create_root_pane() const;

  std::shared_ptr<laf::ComponentUI> create_ui() override;

public:
  void remove(const std::shared_ptr<Component> &c) override;
  void set_layout(const std::shared_ptr<Layout> &layout) override;

  const std::string& get_title() const {
    return this->title;
  }

  void set_title(const std::string &title) {
    this->title = title;
  }

  std::shared_ptr<MenuBar> get_menu_bar() const;
  void set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);

  std::shared_ptr<RootPane> get_root_pane() const override final;
  std::shared_ptr<Component> get_content_pane() const override final;
  void set_content_pane(const std::shared_ptr<Component> &content_pane) override final;
  std::shared_ptr<LayeredPane> get_layered_pane() const override final;
  void set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane) override final;
  std::shared_ptr<Component> get_glass_pane() const override final;
  void set_glass_pane(const std::shared_ptr<Component> &glass_pane) override final;

  void set_root_pane(const std::shared_ptr<RootPane> &root_pane);

  std::shared_ptr<laf::FrameUI> get_ui() const;
};

}
