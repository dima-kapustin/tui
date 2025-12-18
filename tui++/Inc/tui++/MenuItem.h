#pragma once

#include <tui++/MenuElement.h>
#include <tui++/AbstractButton.h>

namespace tui {
namespace laf {
class MenuItemUI;
}

class MenuItem: public AbstractButton, public MenuElement {
  using base = AbstractButton;

  Property<std::optional<KeyStroke>> accelerator { this, "accelerator" };

public:
  std::shared_ptr<laf::MenuItemUI> get_ui() const;

  void process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  void process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  void menu_selection_changed(bool is_included) override;

  std::vector<std::shared_ptr<MenuElement>> get_sub_elements() override;

  std::shared_ptr<Component> get_component() override;

  void set_enabled(bool value) override;

  bool is_armed() const {
    return get_model()->is_armed();
  }

  void set_armed(bool value) {
    get_model()->set_armed(value);
  }

  void set_model(std::shared_ptr<ButtonModel> const &model) override;

  std::optional<KeyStroke> const& get_accelerator() const {
    return this->accelerator;
  }

  void set_accelerator(KeyStroke const &accelerator) {
    if (this->accelerator != accelerator) {
      this->accelerator = accelerator;
      repaint();
      revalidate();
    }
  }

protected:
  MenuItem(std::string const &text);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  void init() override;
  std::shared_ptr<laf::ComponentUI> create_ui() override;

  void process_event(FocusEvent &e) override;

  bool always_on_top() const override {
    return true;
  }
};

}
