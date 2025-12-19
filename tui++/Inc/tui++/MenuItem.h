#pragma once

#include <tui++/MenuElement.h>
#include <tui++/AbstractButton.h>

#include <tui++/event/MenuKeyEvent.h>
#include <tui++/event/MenuDragMouseEvent.h>

namespace tui {
namespace laf {
class MenuItemUI;
}

class MenuItem: public ComponentExtension<AbstractButton, MenuKeyEvent, MenuDragMouseEvent<MousePressEvent>, MenuDragMouseEvent<MouseDragEvent>, MenuDragMouseEvent<MouseOverEvent>>, public MenuElement {
  using base = ComponentExtension<AbstractButton, MenuKeyEvent, MenuDragMouseEvent<MousePressEvent>, MenuDragMouseEvent<MouseDragEvent>, MenuDragMouseEvent<MouseOverEvent>>;

  bool is_mouse_dragged = false;
  Property<std::optional<KeyStroke>> accelerator { this, "accelerator" };

public:
  std::shared_ptr<laf::MenuItemUI> get_ui() const;

  virtual void process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  virtual void process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  virtual void menu_selection_changed(bool is_included) override;

  virtual std::vector<std::shared_ptr<MenuElement>> get_sub_elements() const override;

  virtual std::shared_ptr<Component> get_component() const override;

  virtual void set_enabled(bool value) override;

  virtual void set_model(std::shared_ptr<ButtonModel> const &model) override;

  bool is_armed() const {
    return get_model()->is_armed();
  }

  void set_armed(bool value) {
    get_model()->set_armed(value);
  }

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
  MenuItem(std::string const &text = "") :
      base(text) {
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual void init() override;
  virtual void init_focusability();
  virtual std::shared_ptr<laf::ComponentUI> create_ui() override;

  using base::process_event;
  virtual void process_event(FocusEvent &e) override;
  virtual void process_event(MenuKeyEvent &e) override;
  virtual void process_event(MenuDragMouseEvent<MousePressEvent> &e) override;
  virtual void process_event(MenuDragMouseEvent<MouseDragEvent> &e) override;
  virtual void process_event(MenuDragMouseEvent<MouseOverEvent> &e) override;

  virtual bool always_on_top() const override {
    return true;
  }

  virtual void configure_properties_from_action() override;
  virtual void action_property_changed(PropertyChangeEvent &e) override;
};

}
