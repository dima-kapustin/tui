#pragma once

#include <tui++/Component.h>
#include <tui++/MenuElement.h>
#include <tui++/SingleSelectionModel.h>

#include <tui++/event/PopupMenuEvent.h>
#include <tui++/event/MenuKeyEvent.h>

namespace tui {
namespace laf {
class PopupMenuUI;
}

class Popup;
class MenuItem;

class PopupMenu: public ComponentExtension<Component, PopupMenuEvent, MenuKeyEvent>, public MenuElement {
  using base = ComponentExtension<Component, PopupMenuEvent, MenuKeyEvent>;

  Property<std::string> label { this, "label" };
  std::shared_ptr<SingleSelectionModel> selection_model = std::make_shared<SingleSelectionModel>();

  std::shared_ptr<Component> invoker;
  std::shared_ptr<Frame> frame;
  std::shared_ptr<Popup> popup;

  Point desired_location;

public:
  std::shared_ptr<laf::PopupMenuUI> get_ui() const;

  using base::add;
  std::shared_ptr<MenuItem> add(std::string const &label);
  std::shared_ptr<MenuItem> add(std::shared_ptr<Action> const &action);

  // A row of a concrete kind -- a check box or radio row, a submenu -- keeps
  // its own type: a shared_ptr<Derived> converts both to shared_ptr<MenuItem>
  // (the overload a row wants) and to shared_ptr<Component> (the container's
  // own add), and the two user-defined conversions are indistinguishable to
  // overload resolution. Matching the derived type here, where it is still
  // known, keeps `popup->add(row)` unambiguous -- and hands the row back with
  // the type it was passed in.
  template<typename T>
  requires (std::derived_from<T, MenuItem>)
  std::shared_ptr<T> add(std::shared_ptr<T> const &menu_item) {
    base::add(std::static_pointer_cast<Component>(menu_item));
    return menu_item;
  }

  void add_separator();

  void insert(std::shared_ptr<Component> const &c, size_t index);
  std::shared_ptr<MenuItem> insert(std::shared_ptr<Action> const &action, size_t index);

  std::string const& get_label() const {
    return this->label;
  }

  void set_label(std::string const &label);

  virtual void process_mouse_event(MouseEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  virtual void process_key_event(KeyEvent &e, std::vector<std::shared_ptr<MenuElement>> const &path, std::shared_ptr<MenuSelectionManager> const &manager) override;

  virtual void menu_selection_changed(bool is_included) override;

  virtual std::vector<std::shared_ptr<MenuElement>> get_sub_elements() const override;

  virtual std::shared_ptr<Component> get_component() const override;

  std::shared_ptr<Component> get_invoker() const {
    return this->invoker;
  }

  void set_invoker(std::shared_ptr<Component> const &invoker);

  // Whether the popup window is currently on the screen. The component's own
  // `visible` stays true (hiding it would also hide the popup from its
  // parent's layout, collapsing the popup to zero size); the shown state is
  // the popup window's instead, which can go off the screen behind the
  // menu's back (see Screen::hide_window).
  bool is_popup_showing() const;

  // Whether this menu is showing as a component's context menu: the menu the
  // popup trigger -- a right click, Shift+F10 -- opened through
  // Component::show_component_popup_menu. The widget's own dropdowns are
  // PopupMenus too (the combo box's list puts itself on the selection path
  // like any popup menu), but they are not context menus: the widget drives
  // them itself (the arrow toggles the dropdown, its Escape cancels the
  // edit), and the menu system must keep its hands off them.
  bool is_context_menu() const {
    return this->shown_as_context_menu;
  }

  void set_context_menu(bool value) {
    this->shown_as_context_menu = value;
  }

  using base::show;
  void show(std::shared_ptr<Component> const &invoker, int x, int y);

  std::shared_ptr<PopupMenu> get_root_popup_menu() const;

  virtual void set_visible(bool value) override;
  using base::set_location;
  virtual void set_location(int x, int y) override;

  std::shared_ptr<SingleSelectionModel> get_selection_model() {
    return this->selection_model;
  }

  void set_selection_model(std::shared_ptr<SingleSelectionModel> const &model) {
    this->selection_model = model;
  }

protected:
  PopupMenu(std::string const &label = "") {
    this->label = label;
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual std::shared_ptr<laf::ComponentUI> create_ui() override;

  virtual bool always_on_top() const override {
    return true;
  }

  bool is_popup_menu() const;
  std::shared_ptr<MenuItem> create_item(std::shared_ptr<Action> const &action);

private:
  void show_popup();

  // Set while the menu is showing as a component's context menu (see
  // is_context_menu); cleared when the popup goes down.
  bool shown_as_context_menu = false;

  // Everything a popup menu does when it stops showing: the submenus hanging
  // off it are dismissed, the selection is cleared, BECOMES_INVISIBLE is
  // fired, the popup window is let go and the item highlights are dropped.
  // set_visible(false) calls it, and it is what cleans up after a popup whose
  // window the screen took down with the window it was shown over (see
  // Screen::hide_window): is_popup_showing() already reports that popup as
  // gone, so the next hide -- an application closing its popup, a menu
  // session dropping a stale one -- puts its state to rest through here.
  void drop_popup();
};

}
