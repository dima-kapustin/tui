#pragma once

#include <tui++/Component.h>

namespace tui {
namespace laf {
class RootPaneUI;
}

class Button;
class MenuBar;
class LayeredPane;

class RootPane: public Component {
  using base = Component;
public:
  enum WindowDecorationStyle {
    NONE = 0,
    FRAME = 1,
    PLAIN_DIALOG = 2,
    INFORMATION_DIALOG = 3,
    ERROR_DIALOG = 4,
    COLOR_CHOOSER_DIALOG = 5,
    FILE_CHOOSER_DIALOG = 6,
    QUESTION_DIALOG = 7,
    WARNING_DIALOG = 8
  };

public:
  virtual bool is_validate_root() const override {
    return true;
  }

  std::shared_ptr<MenuBar> get_menu_bar() const {
    return this->menu_bar;
  }
  void set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);

  std::shared_ptr<Component> get_content_pane() const {
    return this->content_pane;
  }
  void set_content_pane(const std::shared_ptr<Component> &content_pane);

  std::shared_ptr<LayeredPane> get_layered_pane() const {
    return this->layered_pane;
  }
  void set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane);

  std::shared_ptr<Component> get_glass_pane() const {
    return this->glass_pane;
  }
  void set_glass_pane(const std::shared_ptr<Component> &glass_pane);

  std::shared_ptr<Button> get_default_button() const {
    return this->default_button;
  }
  void set_default_dutton(const std::shared_ptr<Button> &button);

  WindowDecorationStyle get_window_decoration_style() const {
    return this->window_decoration_style;
  }

  void set_window_decoration_style(WindowDecorationStyle window_decoration_style) {
    this->window_decoration_style = window_decoration_style;
  }

  std::shared_ptr<laf::RootPaneUI> get_ui() const;

protected:
  RootPane() {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  std::shared_ptr<laf::ComponentUI> create_ui() override;

  virtual void init() override;

  virtual void add_notify() override;

  virtual std::shared_ptr<Component> create_class_pane() const;
  virtual std::shared_ptr<LayeredPane> create_layered_pane() const;
  virtual std::shared_ptr<Component> create_content_pane() const;
  virtual std::shared_ptr<Layout> create_root_layout() const;

private:
  Property<WindowDecorationStyle> window_decoration_style { this, "window-decoration-style", WindowDecorationStyle::NONE };

  std::shared_ptr<MenuBar> menu_bar;
  std::shared_ptr<Component> content_pane;
  std::shared_ptr<LayeredPane> layered_pane;
  std::shared_ptr<Component> glass_pane;
  Property<std::shared_ptr<Button>> default_button { this, "default-button" };

  friend class RootLayout;
};

class RootPaneContainer {
protected:
  virtual ~RootPaneContainer() {
  }

public:
  virtual std::shared_ptr<RootPane> get_root_pane() const = 0;
  virtual std::shared_ptr<Component> get_content_pane() const = 0;
  virtual void set_content_pane(const std::shared_ptr<Component> &content_pane) = 0;
  virtual std::shared_ptr<LayeredPane> get_layered_pane() const = 0;
  virtual void set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane) = 0;
  virtual std::shared_ptr<Component> get_glass_pane() const = 0;
  virtual void set_glass_pane(const std::shared_ptr<Component> &glass_pane) = 0;
};

std::shared_ptr<RootPane> get_root_pane(const std::shared_ptr<Component> &c);

}
