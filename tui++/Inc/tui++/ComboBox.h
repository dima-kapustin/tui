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

  // The text being edited (editable combo): the caret is a code point index
  // into `draft`. When no edit is in flight the field shows the selected
  // item instead.
  std::u32string draft;
  size_t draft_caret = 0;
  bool is_editing = false;
  // Whether the user moved the caret (mouse click, arrow keys) since the
  // edit session began. Without it, the first typed letter replaces the
  // selection's text -- the "type to look up" gesture -- instead of
  // appending to the end of it.
  bool caret_moved = false;

  // The letters a non-editable combo collected for its prefix lookup; stale
  // once the combo's "type-ahead" delay (as Swing's KeySelectionManager
  // resets its buffer) has passed.
  std::u32string lookup_buffer;
  EventClock::time_point lookup_buffer_time;

  // The custom text of the last commit that matched no item (Enter with a
  // draft no item starts with). The index model cannot hold it, so it stays
  // unselected but the field keeps showing it until a real selection or a
  // new edit replaces it -- Swing's editable combo leaves the editor's text
  // in place the same way.
  std::u32string committed_text;

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

  virtual void paint(Graphics &g) override;

private:
  std::string selected_item_text() const;
  std::string display_text() const;
  std::string draft_utf8() const;

  void update_preferred_size();
  void update_field_from_model();

  void model_changed(ChangeEvent &e);

  void show_popup();
  void hide_popup(bool revert_draft);
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

  // Starts an edit session from the current field text when none is in
  // flight, leaving the caret at the end.
  void begin_editing_if_needed();

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

  // Applies a draft-editing key (Backspace, Delete, the caret arrows, Home,
  // End) to the editable field's draft; see ComboBox.cpp for the split of
  // editing keys and popup keys while the dropdown is open.
  void handle_draft_edit_key(KeyEvent::KeyCode code);

  // Scrolls the dropdown's window when the mouse wheel turns over the open
  // popup (the listener lives on the popup menu, the rows' parent).
  void popup_menu_wheel_moved(MouseWheelEvent &e);
};

}
