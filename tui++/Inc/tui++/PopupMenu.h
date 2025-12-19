#pragma once

#include <tui++/Component.h>
#include <tui++/MenuElement.h>
#include <tui++/Popup.h>

namespace tui {
namespace laf {
class PopupMenuUI;
}

class PopupMenu: public Component, public MenuElement {
  using base = Component;

  Property<std::string> label { this, "label" };

  std::shared_ptr<Component> invoker;
  std::shared_ptr<Frame> frame;
  std::shared_ptr<Popup> popup;

public:
  std::shared_ptr<laf::PopupMenuUI> get_ui() const;

  virtual void process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  virtual void process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  virtual void menu_selection_changed(bool is_included) override;

  virtual std::vector<std::shared_ptr<MenuElement>> get_sub_elements() const override;

  virtual std::shared_ptr<Component> get_component() const override;

  std::shared_ptr<Component> get_invoker() const {
    return this->invoker;
  }

  void set_invoker(std::shared_ptr<Component> const& invoker);

  using base::show;
  void show(std::shared_ptr<Component> const& invoker, int x, int y);

protected:
  PopupMenu(std::string const &label = "") {
    this->label = label;
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual std::shared_ptr<laf::ComponentUI> create_ui() override;
};

}
