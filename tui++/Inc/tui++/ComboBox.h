#pragma once

#include <tui++/Component.h>
#include <tui++/Graphics.h>

#include <tui++/event/ActionEvent.h>
#include <tui++/event/ChangeEvent.h>
#include <tui++/event/Event.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace tui {

class ComboBoxModel;
class PopupMenu;
class TextField;
class TextMetrics;

// Swing's JComboBox: a button-and-field widget that shows the selected item
// and pops up a list of items below it. The items live in a ComboBoxModel
// (a DefaultComboBoxModel unless set_model replaces it).
//
// Behavior follows Swing:
//   - a click on the arrow (or anywhere on a non-editable combo) toggles the
//     dropdown; a click elsewhere closes it,
//   - Up/Down/Page/Home/End move the popup highlight (wrapping around),
//     Enter picks the highlighted row and fires an ActionEvent, Escape
//     dismisses the popup,
//   - the field is directly editable when set_editable(true) ("lookup
//     editing"): typing edits the text, the popup selection follows the
//     typed prefix (the first match from the top is highlighted, live, after
//     every change) and Enter commits the matching item -- or keeps the typed
//     text as a custom value when no item matches (the field then shows that
//     value, unselected). Enter with nothing to commit is a no-op, so a
//     stray or repeated Enter after a commit cannot discard the selection.
//     Escape reverts an uncommitted edit. A non-editable combo selects the
//     item matching the letters typed.
//
// The dropdown closes when the arrow is clicked again, when Enter commits a
// pick, when the combo loses the focus, or on a mouse press anywhere outside
// the combo and its dropdown.
//
// A programmatic selection change (set_selected_index) fires no ActionEvent;
// user picks and Enter commits do.
//
// The editable field is a TextField child (Swing's BasicComboBoxEditor holds a
// JTextField), laid out over the field area: it owns the text, the caret and
// the selection, so the editing keys, the selection gestures and the clipboard
// commands in the editor are the text component's. The combo keeps the
// keyboard focus and forwards its keys to the editor (see on_key_pressed),
// drives the editor's caret through set_editing_focus, and reads the editor's
// text for the live lookup and the Enter commits.
class ComboBox: public ComponentExtension<Component, ActionEvent> {
  using base = ComponentExtension<Component, ActionEvent>;

  Property<std::shared_ptr<ComboBoxModel>> model { this, "Model" };
  Property<bool> editable { this, "Editable", false };
  Property<int> maximum_row_count { this, "MaximumRowCount", 8 };

  ChangeListener model_change_listener = std::bind(&ComboBox::model_changed, this, std::placeholders::_1);

  // The dropdown and its rows. The popup menu is created on first use and
  // re-populated whenever it is shown.
  std::shared_ptr<PopupMenu> popup_menu;
  bool popup_visible = false;
  size_t popup_scroll_start = 0;
  std::optional<size_t> popup_armed_index;
  std::optional<size_t> lookup_index;

  // The text the field is editing (editable combo): a TextField child owning
  // the caret and the selection. While the combo is not editable the editor
  // is hidden and the combo paints the selected item itself.
  std::shared_ptr<TextField> editor;

  // The text the combo last put into the editor (the selected item, or a
  // committed custom value). It is what Escape reverts to and what tells an
  // edit in flight apart from a programmatic field update -- Swing's
  // "editor modified" test.
  std::string editor_baseline;

  // Guards the editor's change listener while the combo updates the field
  // itself (a programmatic set_text must not run the lookup as a user edit).
  bool setting_editor_text = false;

  // The letters a non-editable combo collected for its prefix lookup; stale
  // once the combo's "type-ahead" delay (as Swing's KeySelectionManager
  // resets its buffer) has passed.
  std::u32string lookup_buffer;
  EventClock::time_point lookup_buffer_time;

  friend class ComboBoxKeyForwarder;
  friend class ComboBoxDismissObserver;

  std::shared_ptr<class ComboBoxKeyForwarder> key_forwarder;
  std::shared_ptr<class ComboBoxDismissObserver> dismiss_observer;

public:
  // ---- model ----

  std::shared_ptr<ComboBoxModel> get_model() const {
    return this->model;
  }

  void set_model(std::shared_ptr<ComboBoxModel> const &model);

  // ---- items (delegate to the model; a DefaultComboBoxModel is created
  // with the combo, so the mutations are always available) ----

  size_t get_item_count() const;
  std::string get_item_at(size_t index) const;

  void add_item(std::string const &item);
  void insert_item_at(std::string const &item, size_t index);
  void remove_item_at(size_t index);
  void remove_all_items();

  // ---- selection ----

  std::optional<size_t> get_selected_index() const;
  void set_selected_index(std::optional<size_t> index);

  // The text of the selected item, empty when nothing is selected.
  std::string get_selected_item() const;

  // Selects the item whose text equals `item`; a no-op when the model has no
  // such item.
  void set_selected_item(std::string const &item);

  // ---- editing / look & feel ----

  bool is_editable() const {
    return this->editable;
  }

  // Swing's setEditable: whether the field accepts typing. The typed text is
  // looked up against the items while the dropdown is open and committed by
  // Enter.
  void set_editable(bool value);

  // The editor field of an editable combo (Swing's getEditor: the
  // BasicComboBoxEditor that wraps a text field), null while the combo is not
  // editable.
  std::shared_ptr<TextField> get_editor() const {
    return this->editable ? this->editor : nullptr;
  }

  int get_maximum_row_count() const {
    return this->maximum_row_count;
  }

  // Swing's setMaximumRowCount: the number of rows the dropdown shows before
  // it starts scrolling.
  void set_maximum_row_count(int count);

  bool is_popup_visible() const {
    return this->popup_visible;
  }

  // Shows or dismisses the dropdown (Swing's setPopupVisible).
  void set_popup_visible(bool value);

  // The text the field currently shows: the draft while it is being edited,
  // else the selected item's text.
  std::string get_field_text() const;

  virtual ~ComboBox();

protected:
  ComboBox();
  ComboBox(std::vector<std::string> items);
  ComboBox(std::shared_ptr<ComboBoxModel> const &model);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual void init() override;
  virtual void add_notify() override;
  virtual void remove_notify() override;

  // Lays the editor field out over the combo's field area, before the tree
  // paints (Swing's BasicComboBoxUI.layoutContainer does the same for its
  // editor).
  virtual void do_layout() override;

  // The combo paints its own face; the editor child is painted by the base
  // class's paint (paint_component, the border, then the children).
  virtual void paint_component(Graphics &g) override;

private:
  std::string selected_item_text() const;
  std::string display_text() const;

  void update_preferred_size();

  // Puts `text` into the editor as the combo's own value: the lookup does not
  // run and the text becomes the baseline the edit-in-flight test and Escape
  // compare against.
  void set_editor_text(std::string const &text);

  // Whether the editor holds text the user typed (anything but the combo's
  // own value): what Enter commits as a custom value.
  bool editor_modified() const;

  void layout_editor();

  void editor_changed();
  void model_changed(ChangeEvent &e);

  void show_popup();
  void hide_popup(bool revert_edit);
  void rebuild_popup_rows();

  // The item index whose text starts with `prefix` (case-insensitive ASCII),
  // or nullopt.
  std::optional<size_t> lookup(std::u32string const &prefix) const;
  void set_lookup_prefix(std::u32string prefix);

  bool field_hit(int x) const;

  // Whether a key event that reached the combo's window belongs to the combo:
  // the combo is the focus owner, or its dropdown is open.
  bool key_events_belong_to_self() const;

  // The window the dropdown currently lives in (the popup menu's containing
  // window), or null when no dropdown is shown.
  std::shared_ptr<Window> popup_window() const;

  void commit_selection(std::optional<size_t> index);
  void fire_action_event(std::string const &text);

  void move_popup_selection(int direction);
  void page_popup_selection(int direction);
  void jump_popup_selection(bool to_end);
  void ensure_popup_armed_visible();

  void on_key_pressed(KeyEvent &e);
  void on_key_typed(KeyEvent &e);
  void on_mouse_pressed(MousePressEvent &e);
  void on_focus_changed(bool gained);

  // Scrolls the dropdown's window when the mouse wheel turns over the open
  // popup (the listener lives on the popup menu, the rows' parent).
  void popup_menu_wheel_moved(MouseWheelEvent &e);
};

}
