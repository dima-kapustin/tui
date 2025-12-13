#pragma once

#include <tui++/Button.h>
#include <tui++/MenuBar.h>
#include <tui++/LayeredPane.h>

namespace tui {

class RootPane: public Component {
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
  void set_menu_bar(const std::shared_ptr<MenuBar> &menu_bar);
  void set_content_pane(const std::shared_ptr<Component> &content_pane);
  void set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane);
  void set_glass_pane(const std::shared_ptr<Component> &glass_pane);
  void set_default_dutton(const std::shared_ptr<Button> &default_dutton);

protected:
  virtual void init() override;

  virtual std::shared_ptr<Component> create_class_pane();
  virtual std::shared_ptr<LayeredPane> create_layered_pane();
  virtual std::shared_ptr<Component> create_content_pane();
  virtual std::shared_ptr<Layout> create_root_layout();

private:
  Property<WindowDecorationStyle> window_decoration_style { this, "window_decoration_style", WindowDecorationStyle::NONE};

  std::shared_ptr<MenuBar> menu_bar;
  std::shared_ptr<Component> content_pane;
  std::shared_ptr<LayeredPane> layered_pane;
  std::shared_ptr<Component> glass_pane;
  std::shared_ptr<Button> default_dutton;

  friend class RootLayout;
};

}
