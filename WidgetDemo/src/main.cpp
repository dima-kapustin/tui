// Widget demo for tui++: the Swing-style toggleable widgets and the combo
// box on one frame.
//
//     WidgetDemo            text screen  (cell-based escape sequences)
//     WidgetDemo text       same as the default
//     WidgetDemo sixel      sixel screen (pixel-based graphics)
//
// What to try:
//   - click the check boxes, radio buttons and the toggle button; radio
//     buttons and the grouped check boxes are exclusive per group,
//   - the View menu holds a check box menu item and a Density submenu whose
//     radio items are grouped,
//   - the two combo boxes: arrow keys open the dropdown, Up/Down/Home/End
//     move the highlight, Enter picks, Escape cancels. The "City" combo is
//     editable: typing looks the item up ("lookup editing"), Enter commits
//     the match or keeps custom text; the "Font size" combo is read-only and
//     picks the item you type letters for.
//
// The bottom status line summarizes every control after each change.

#include <tui++/BorderLayout.h>
#include <tui++/BoxLayout.h>
#include <tui++/Button.h>
#include <tui++/ButtonGroup.h>
#include <tui++/CheckBox.h>
#include <tui++/CheckBoxMenuItem.h>
#include <tui++/ComboBox.h>
#include <tui++/Frame.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/Panel.h>
#include <tui++/RadioButton.h>
#include <tui++/RadioButtonMenuItem.h>
#include <tui++/ToggleButton.h>

#include <tui++/terminal/Terminal.h>

#include <memory>
#include <string>

using namespace tui;

namespace tui {

// A one-line panel that paints its text (a heading or the status line).
class WidgetDemoTextLine: public Panel {
  std::string text;

public:
  void set_text(std::string const &text) {
    if (this->text != text) {
      this->text = text;
      repaint();
    }
  }

  void paint(Graphics &g) override {
    if (not this->text.empty()) {
      g.draw_string(this->text, 0, 0);
    }
  }

protected:
  WidgetDemoTextLine(std::string const &text = "") :
      text(text) {
    set_preferred_size(Dimension { 0, 1 });
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

// A horizontal row of the demo's control panel.
class WidgetDemoRow: public Panel {
protected:
  WidgetDemoRow() = default;

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);
};

std::string widget_on_off(bool value) {
  return value ? "on" : "off";
}

std::string widget_selected_text(std::shared_ptr<AbstractButton> const &button) {
  return button->is_selected() ? button->get_text() : std::string { };
}

}

using namespace tui;

int main(int argc, char *argv[]) {
  auto type = argc > 1 ? std::string(argv[1]) : std::string { "text" };
  terminal.set_type(type);

  auto frame = make_component<Frame>();
  frame->set_size({ 80, 24 });

  // ---- the controls whose state the status line summarizes ----
  auto bold = make_component<ToggleButton>("Bold");
  auto italic = make_component<CheckBox>("Italic");
  auto underline = make_component<CheckBox>("Underline");

  auto alignment_group = std::make_shared<ButtonGroup>();
  auto align_left = make_component<RadioButton>("Align Left");
  auto align_center = make_component<RadioButton>("Align Center");
  auto align_right = make_component<RadioButton>("Align Right");
  alignment_group->add(align_left);
  alignment_group->add(align_center);
  alignment_group->add(align_right);

  // A group of check boxes: the group only arbitrates NEW selections, so
  // clicking the selected member clears it again (Swing semantics).
  auto toggles_group = std::make_shared<ButtonGroup>();
  auto snap = make_component<CheckBox>("Snap to grid");
  auto guides = make_component<CheckBox>("Show guides");
  toggles_group->add(snap);
  toggles_group->add(guides);

  auto city = make_component<ComboBox>(std::vector<std::string> { "Paris", "London", "Rome", "Berlin", "Madrid", "Amsterdam", "Prague", "Vienna" });
  city->set_editable(true);
  city->set_selected_index(0);

  auto size = make_component<ComboBox>();
  for (auto i = 8; i <= 24; i += 2) {
    size->add_item(std::to_string(i) + " pt");
  }
  size->set_maximum_row_count(5);
  size->set_selected_index(0);

  auto status = make_component<WidgetDemoTextLine>();
  auto refresh_status = [=] {
    auto text = std::string { "bold=" + widget_on_off(bold->is_selected()) + " italic=" + widget_on_off(italic->is_selected()) + " underline=" + widget_on_off(underline->is_selected()) };
    text += " | align=" + widget_selected_text(align_left) + widget_selected_text(align_center) + widget_selected_text(align_right);
    text += " | snap=" + widget_on_off(snap->is_selected()) + " guides=" + widget_on_off(guides->is_selected());
    text += " | city=" + city->get_field_text() + " size=" + size->get_selected_item();
    status->set_text(text);
  };
  auto notify_all = [refresh_status](auto const &component) {
    component->add_listener([refresh_status](ActionEvent &) {
      refresh_status();
    });
  };
  notify_all(bold);
  notify_all(italic);
  notify_all(underline);
  notify_all(align_left);
  notify_all(align_center);
  notify_all(align_right);
  notify_all(snap);
  notify_all(guides);
  notify_all(city);
  notify_all(size);

  // ---- the menu bar: check and radio menu items, and a submenu ----
  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  auto quit = make_component<MenuItem>("Quit", Char { 'x' });
  quit->add_listener([](ActionEvent &) {
    terminal.shutdown();
  });
  file_menu->add(quit);

  auto view_menu = make_component<Menu>("View");
  view_menu->set_mnemonic('V');
  auto wrap = make_component<CheckBoxMenuItem>("Word wrap");
  wrap->add_listener([refresh_status](ActionEvent &) {
    refresh_status();
  });
  view_menu->add(wrap);
  view_menu->add_separator();

  // The density radio group hangs under a submenu of the View popup: a
  // nested Menu is a row of the popup that opens its own popup (Swing's
  // JMenu in a JPopupMenu), the radio items stay exclusive as before.
  auto density_menu = make_component<Menu>("Density");
  auto density_group = std::make_shared<ButtonGroup>();
  auto density = std::vector<std::shared_ptr<RadioButtonMenuItem>> { };
  for (auto &&label : { "Compact", "Comfortable", "Roomier" }) {
    auto item = make_component<RadioButtonMenuItem>(label);
    density_group->add(item);
    item->add_listener([refresh_status](ActionEvent &) {
      refresh_status();
    });
    density_menu->add(item);
    density.push_back(item);
  }
  density[0]->set_selected(true);
  view_menu->add(density_menu);

  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(file_menu);
  menu_bar->add(view_menu);
  frame->set_menu_bar(menu_bar);

  // Demo-style popup wiring (see the MenuBar demo): a click on a top-level
  // menu opens its popup and closes the other; a click on the content area
  // dismisses an open popup.
  for (auto &&menu : { file_menu, view_menu }) {
    auto weak_self = std::weak_ptr<Menu> { menu };
    auto weak_others = std::vector<std::weak_ptr<Menu>> { };
    for (auto &&other : { file_menu, view_menu }) {
      if (other != menu) {
        weak_others.emplace_back(other);
      }
    }
    menu->add_listener([weak_self, weak_others](MousePressEvent &e) {
      if (e.id != MousePressEvent::MOUSE_RELEASED) {
        return;
      }
      auto self = weak_self.lock();
      if (not self) {
        return;
      }
      if (self->is_popup_menu_visible()) {
        self->set_popup_menu_visible(false);
      } else {
        for (auto &&weak_other : weak_others) {
          if (auto other = weak_other.lock()) {
            other->set_popup_menu_visible(false);
          }
        }
        self->set_popup_menu_visible(true);
      }
      self->set_armed(true);
    });
  }

  // ---- the control panel ----
  auto content = frame->get_content_pane();
  content->set_layout(std::make_shared<BorderLayout>());

  auto panel = make_component<Panel>();
  panel->set_layout(std::make_shared<BoxLayout>(panel.get(), BoxLayout::Y));

  auto heading = make_component<WidgetDemoTextLine>("Text style (toggle button, check boxes):");
  auto row1 = make_component<WidgetDemoRow>();
  row1->add(bold);
  row1->add(italic);
  row1->add(underline);
  panel->add(heading);
  panel->add(row1);

  auto heading2 = make_component<WidgetDemoTextLine>("Alignment (radio buttons in one group):");
  auto row2 = make_component<WidgetDemoRow>();
  row2->add(align_left);
  row2->add(align_center);
  row2->add(align_right);
  panel->add(heading2);
  panel->add(row2);

  auto heading3 = make_component<WidgetDemoTextLine>("Snapping (grouped check boxes):");
  auto row3 = make_component<WidgetDemoRow>();
  row3->add(snap);
  row3->add(guides);
  panel->add(heading3);
  panel->add(row3);

  auto heading4 = make_component<WidgetDemoTextLine>("Combo boxes (editable lookup / dropdown):");
  auto row4 = make_component<WidgetDemoRow>();
  row4->add(city);
  row4->add(size);
  panel->add(heading4);
  panel->add(row4);

  content->add(panel, BorderLayout::CENTER);
  content->add(status, BorderLayout::SOUTH);

  // A click outside an open popup dismisses it.
  content->add_listener([file_menu, view_menu](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    for (auto &&menu : { file_menu, view_menu }) {
      if (menu->is_popup_menu_visible()) {
        menu->set_popup_menu_visible(false);
      }
    }
  });

  refresh_status();
  terminal.set_title("tui++ widget demo - " + type);
  frame->set_visible(true);
  terminal.run_event_loop();
}
