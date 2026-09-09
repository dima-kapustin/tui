#include <tui++/ComboBox.h>

#include <tui++/ComboBoxModel.h>
#include <tui++/DefaultComboBoxModel.h>
#include <tui++/Char.h>
#include <tui++/CharIterator.h>
#include <tui++/Insets.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/MenuItem.h>
#include <tui++/PopupMenu.h>
#include <tui++/Screen.h>
#include <tui++/Symbols.h>
#include <tui++/TextMetrics.h>
#include <tui++/Window.h>

#include <tui++/event/FocusEvent.h>
#include <tui++/lookandfeel/LookAndFeel.h>

#include <cassert>
#include <cstddef>

#include <algorithm>

namespace tui {

namespace {

// The arrow strip at the right edge of the combo box: one glyph column of
// breathing room plus the arrow glyph's column.
constexpr int ARROW_GAP_COLUMNS = 1;
constexpr int ARROW_GLYPH_COLUMNS = 1;

constexpr auto TYPE_AHEAD_DELAY = std::chrono::milliseconds(800);

// ASCII case folding, as the rest of the toolkit folds mnemonic letters.
char32_t fold_case(char32_t code) {
  return code >= 'A' and code <= 'Z' ? code - 'A' + 'a' : code;
}

// A character the editor accepts: plain (optionally shifted) printable
// characters; Ctrl/Alt/Meta chords belong to action maps.
bool is_printable(KeyEvent const &e) {
  if (e.modifiers & (InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN)) {
    return false;
  }
  auto code = e.get_key_char().get_code();
  return code >= 0x20 and code != 0x7f;
}

std::u32string to_u32(std::string_view text) {
  auto result = std::u32string { };
  result.reserve(text.size());
  for (auto it = to_chars(text), last = end(it); it != last; ++it) {
    result.push_back((*it).get_code());
  }
  return result;
}

std::string to_utf8(std::u32string const &text) {
  auto result = std::string { };
  result.reserve(text.size());
  for (auto code : text) {
    auto glyph = Char(code);
    result.append(std::string_view(glyph));
  }
  return result;
}

}

// ---------------------------------------------------------------------------
// Key forwarder. A key event is dispatched to the focused *window*; the
// combo listens on its window and takes the events while it is the focus
// owner or its dropdown is open (Swing's combo owns the keys of its popup).
class ComboBoxKeyForwarder final: public EventListener<KeyEvent> {
  std::weak_ptr<ComboBox> combo;

public:
  explicit ComboBoxKeyForwarder(std::weak_ptr<ComboBox> const &combo) :
      combo(combo) {
  }

  virtual void key_pressed(KeyEvent &e) override {
    if (not e.consumed) {
      if (auto combo = this->combo.lock(); combo and combo->key_events_belong_to_self()) {
        combo->on_key_pressed(e);
      }
    }
  }

  virtual void key_typed(KeyEvent &e) override {
    if (not e.consumed) {
      if (auto combo = this->combo.lock(); combo and combo->key_events_belong_to_self()) {
        combo->on_key_typed(e);
      }
    }
  }
};

// Watches mouse presses while the dropdown is open: a press anywhere outside
// the combo and its dropdown dismisses the popup (Swing's BasicComboPopup
// hides on any outside press).
class ComboBoxDismissObserver final: public EventListener<Event> {
  std::weak_ptr<ComboBox> combo;

public:
  explicit ComboBoxDismissObserver(std::weak_ptr<ComboBox> const &combo) :
      combo(combo) {
  }

  virtual void event_dispatched(Event &e) override {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    auto combo = this->combo.lock();
    if (not combo or not combo->popup_visible) {
      return;
    }
    auto &press = static_cast<MousePressEvent&>(e);
    auto source = std::dynamic_pointer_cast<Component>(press.source);
    if (not source) {
      return;
    }

    // A press on the combo itself (its own handler toggles the popup) or
    // inside the dropdown's popup window (a row pick there hides it) leaves
    // the popup alone.
    if (source.get() == combo.get() or source->get_containing_window() == combo->popup_window()) {
      return;
    }
    combo->set_popup_visible(false);
  }
};

// ---------------------------------------------------------------------------

ComboBox::ComboBox() :
    ComboBox(std::shared_ptr<ComboBoxModel> { }) {
}

ComboBox::ComboBox(std::vector<std::string> items) :
    ComboBox(std::make_shared<DefaultComboBoxModel>(std::move(items))) {
}

ComboBox::ComboBox(std::shared_ptr<ComboBoxModel> const &model) {
  this->model = model ? model : std::make_shared<DefaultComboBoxModel>();
}

ComboBox::~ComboBox() = default;

void ComboBox::init() {
  base::init();

  this->model->add_listener(this->model_change_listener);
  this->popup_menu = make_component<PopupMenu>();

  // The mouse wheel over the open dropdown scrolls its window. The wheel
  // event lands on the hovered row (a MenuItem has no wheel handling), and
  // Component::dispatch_event walks it up to the first wheel-enabled
  // ancestor -- this popup menu. A wheel over the popup's own background
  // reaches the popup directly.
  this->popup_menu->add_listener([this](MouseWheelEvent &e) {
    popup_menu_wheel_moved(e);
  });

  // The dropdown's window can close behind the combo's back: opening another
  // combo's dropdown (or any other popup session) replaces the menu
  // selection path, and the manager hides this popup with it. Resync the
  // combo's state then, so it neither keeps the armed row of a dropdown that
  // is gone nor keeps holding the window's keys (a combo whose dropdown was
  // closed this way used to stay "open" in its own bookkeeping, deadlocking
  // the arrows and the keyboard of every combo around it). The window itself
  // is hidden by whoever dismissed it -- never re-hide it from inside its own
  // hide, or the hide recurses.
  this->popup_menu->add_listener([this](PopupMenuEvent &e) {
    if (e.id == PopupMenuEvent::BECOMES_INVISIBLE and this->popup_visible) {
      this->popup_visible = false;
      this->popup_armed_index = std::nullopt;
      if (this->dismiss_observer) {
        screen.remove_listener(this->dismiss_observer);
        this->dismiss_observer.reset();
      }
      repaint();
    }
  });

  // A press on the field or the arrow owns the mouse: it focuses the combo
  // and toggles/places the dropdown.
  add_listener(MousePressEvent::MOUSE_PRESSED, [this](MousePressEvent &e) {
    on_mouse_pressed(e);
  });
  add_listener([this](FocusEvent &e) {
    on_focus_changed(e.id == FocusEvent::FOCUS_GAINED);
  });

  update_field_from_model();
  update_preferred_size();
}

void ComboBox::add_notify() {
  if (auto window = get_containing_window()) {
    this->key_forwarder = std::make_shared<ComboBoxKeyForwarder>(std::static_pointer_cast<ComboBox>(shared_from_this()));
    window->add_listener(this->key_forwarder);
  }
  base::add_notify();
}

void ComboBox::remove_notify() {
  hide_popup(false);
  if (auto window = get_containing_window(); window and this->key_forwarder) {
    window->remove_listener(this->key_forwarder);
  }
  this->key_forwarder.reset();
  base::remove_notify();
}

bool ComboBox::key_events_belong_to_self() const {
  return this->popup_visible or (is_showing() and is_focus_owner());
}

std::shared_ptr<Window> ComboBox::popup_window() const {
  if (this->popup_menu) {
    return this->popup_menu->get_containing_window();
  }
  return {};
}

// ---- model and items ------------------------------------------------------

void ComboBox::set_model(std::shared_ptr<ComboBoxModel> const &model) {
  if (this->model == model) {
    return;
  }
  if (this->model) {
    this->model->remove_listener(this->model_change_listener);
  }
  this->model = model ? model : std::make_shared<DefaultComboBoxModel>();
  this->model->add_listener(this->model_change_listener);

  // A new model resets the edit state and dismisses the dropdown, as Swing's
  // setModel does.
  this->is_editing = false;
  this->draft.clear();
  this->draft_caret = 0;
  this->caret_moved = false;
  this->lookup_index = std::nullopt;
  this->committed_text.clear();
  set_popup_visible(false);

  update_field_from_model();
  update_preferred_size();
  repaint();
}

size_t ComboBox::get_item_count() const {
  return this->model->get_size();
}

std::string ComboBox::get_item_at(size_t index) const {
  return this->model->get_item_at(index);
}

void ComboBox::add_item(std::string const &item) {
  this->model->add_item(item);
}

void ComboBox::insert_item_at(std::string const &item, size_t index) {
  this->model->insert_item_at(item, index);
}

void ComboBox::remove_item_at(size_t index) {
  this->model->remove_item_at(index);
}

void ComboBox::remove_all_items() {
  this->model->remove_all_items();
}

// ---- selection ------------------------------------------------------------

std::optional<size_t> ComboBox::get_selected_index() const {
  return this->model->get_selected_index();
}

void ComboBox::set_selected_index(std::optional<size_t> index) {
  this->model->set_selected_index(index);
}

std::string ComboBox::get_selected_item() const {
  return selected_item_text();
}

void ComboBox::set_selected_item(std::string const &item) {
  for (auto i = size_t { 0 }, n = this->model->get_size(); i != n; ++i) {
    if (this->model->get_item_at(i) == item) {
      set_selected_index(i);
      return;
    }
  }
}

// ---- look and feel ---------------------------------------------------------

void ComboBox::set_editable(bool value) {
  if (this->editable != value) {
    this->editable = value;
    // Dropping the editable mode discards the draft and the committed custom
    // text; enabling it starts from the current selection's text.
    if (not value) {
      this->is_editing = false;
      this->draft.clear();
      this->draft_caret = 0;
      this->caret_moved = false;
      this->lookup_index = std::nullopt;
      this->committed_text.clear();
    }
    update_field_from_model();
    update_preferred_size();
    repaint();
  }
}

void ComboBox::set_maximum_row_count(int count) {
  if (count < 1) {
    count = 1;
  }
  if (this->maximum_row_count != count) {
    this->maximum_row_count = count;
    if (this->popup_visible) {
      ensure_popup_armed_visible();
      rebuild_popup_rows();
    }
  }
}

void ComboBox::set_popup_visible(bool value) {
  if (value and not this->popup_visible) {
    show_popup();
  } else if (not value and this->popup_visible) {
    // Closing with the mouse or by picking keeps the field as it is; Escape
    // reverts explicitly (hide_popup(true)).
    hide_popup(false);
  }
}

std::string ComboBox::get_field_text() const {
  return display_text();
}

// ---- internal state helpers -------------------------------------------------

std::string ComboBox::selected_item_text() const {
  if (auto index = this->model->get_selected_index()) {
    return this->model->get_item_at(*index);
  }
  return {};
}

std::string ComboBox::display_text() const {
  if (this->is_editing) {
    return draft_utf8();
  }
  if (auto index = this->model->get_selected_index()) {
    return this->model->get_item_at(*index);
  }
  // Nothing selected: the field shows the committed custom value (a text no
  // item matched, kept by Enter), if there is one.
  return to_utf8(this->committed_text);
}

std::string ComboBox::draft_utf8() const {
  return to_utf8(this->draft);
}

void ComboBox::update_field_from_model() {
  if (not this->is_editing) {
    this->draft.clear();
    this->draft_caret = 0;
  }
}

void ComboBox::model_changed(ChangeEvent &e) {
  // Contents and selection changes come through the model. A change the
  // combo itself caused (commit_selection) already cleared the draft; an
  // external change (set_selected_index from the application, model edits)
  // supersedes an edit in flight the way Swing's setSelectedItem does.
  this->is_editing = false;
  this->draft.clear();
  this->draft_caret = 0;
  this->caret_moved = false;
  this->lookup_index = std::nullopt;

  if (this->popup_visible) {
    this->popup_armed_index = std::nullopt;
    rebuild_popup_rows();
  }
  update_preferred_size();
  repaint();
}

// ---- dropdown ---------------------------------------------------------------

void ComboBox::show_popup() {
  if (this->model->get_size() == 0 or this->popup_visible or (this->popup_menu and this->popup_menu->is_popup_showing())) {
    return;
  }

  // Open the popup on the row the user is likely to pick: the current lookup
  // match, else the selection, else the first row.
  this->popup_armed_index = this->lookup_index;
  if (not this->popup_armed_index) {
    this->popup_armed_index = this->model->get_selected_index();
  }
  if (not this->popup_armed_index) {
    this->popup_armed_index = 0;
  }
  this->popup_scroll_start = 0;
  ensure_popup_armed_visible();
  rebuild_popup_rows();

  this->popup_visible = true;

  // The dropdown spans at least the combo box's own width: its rows' natural
  // width (the widest item) is narrower than the box (the arrow strip and
  // the margins), which used to leave the dropdown's right border inset from
  // the box's. A list that must be wider than the box keeps its right edge
  // under the box's arrow and extends to the left -- a native drop-down
  // never sticks out past the box's right border. A combo hugging the
  // screen's left edge falls back to left alignment instead of clipping.
  this->popup_menu->set_preferred_size(std::nullopt); // drop the previous show's forced width
  auto popup_size = this->popup_menu->get_preferred_size();
  auto popup_x = 0;
  if (popup_size.width < get_width()) {
    this->popup_menu->set_preferred_size(Dimension { get_width(), popup_size.height });
  } else if (popup_size.width > get_width()) {
    popup_x = get_width() - popup_size.width;
  }
  auto origin = get_location_on_screen();
  if (origin.x + popup_x < 0) {
    popup_x = -origin.x;
  }

  // The dropdown drops under the combo's field (the popup's x/y are relative
  // to the invoker, like a Swing popup under the combo box).
  this->popup_menu->show(std::static_pointer_cast<Component>(shared_from_this()), popup_x, get_height());

  this->dismiss_observer = std::make_shared<ComboBoxDismissObserver>(std::static_pointer_cast<ComboBox>(shared_from_this()));
  screen.add_listener(EventType::MOUSE_PRESS, this->dismiss_observer);

  repaint();
}

void ComboBox::hide_popup(bool revert_draft) {
  if (not this->popup_visible) {
    return;
  }
  this->popup_visible = false;
  this->popup_armed_index = std::nullopt;

  if (this->popup_menu and this->popup_menu->is_popup_showing()) {
    this->popup_menu->set_visible(false);
  }
  if (this->dismiss_observer) {
    screen.remove_listener(this->dismiss_observer);
    this->dismiss_observer.reset();
  }
  if (revert_draft) {
    this->is_editing = false;
    this->draft.clear();
    this->draft_caret = 0;
    this->caret_moved = false;
    this->lookup_index = std::nullopt;
  }
  repaint();
}

void ComboBox::rebuild_popup_rows() {
  if (not this->popup_menu) {
    return;
  }

  while (this->popup_menu->get_component_count() != 0) {
    this->popup_menu->remove(0);
  }

  auto count = this->model->get_size();
  auto rows = std::min<size_t>(count - this->popup_scroll_start, size_t(this->maximum_row_count));
  for (auto i = size_t { 0 }; i != rows; ++i) {
    auto index = this->popup_scroll_start + i;
    auto item = make_component<MenuItem>(this->model->get_item_at(index));
    if (this->popup_armed_index == index) {
      item->set_armed(true);
    }
    auto weak = std::weak_ptr<ComboBox>(std::static_pointer_cast<ComboBox>(shared_from_this()));
    item->add_listener([weak, index](ActionEvent &) {
      if (auto combo = weak.lock()) {
        combo->commit_selection(index);
      }
    });
    this->popup_menu->add(item);
  }

  if (this->popup_visible) {
    // The rows were replaced while the dropdown is on screen: lay them out
    // right away so mouse hit-testing (and painting) sees the new rows
    // instead of the stale bounds of the removed ones.
    if (auto window = this->popup_menu->get_containing_window()) {
      window->validate();
    }
  }
}

// ---- lookup ---------------------------------------------------------------

std::optional<size_t> ComboBox::lookup(std::u32string const &prefix) const {
  if (prefix.empty()) {
    return std::nullopt;
  }
  for (auto i = size_t { 0 }, n = this->model->get_size(); i != n; ++i) {
    auto item = to_u32(this->model->get_item_at(i));
    if (item.size() < prefix.size()) {
      continue;
    }
    auto matches = true;
    for (auto k = size_t { 0 }; k != prefix.size(); ++k) {
      if (fold_case(item[k]) != fold_case(prefix[k])) {
        matches = false;
        break;
      }
    }
    if (matches) {
      return i;
    }
  }
  return std::nullopt;
}

void ComboBox::set_lookup_prefix(std::u32string prefix) {
  this->lookup_index = lookup(prefix);
  if (this->popup_visible) {
    if (this->lookup_index) {
      this->popup_armed_index = this->lookup_index;
      ensure_popup_armed_visible();
      rebuild_popup_rows();
    } else if (is_editable() and this->popup_armed_index) {
      // Nothing matches the draft: drop the stale highlight, so Enter cannot
      // pick an unrelated row -- the draft commits as a custom value instead.
      this->popup_armed_index = std::nullopt;
      rebuild_popup_rows();
    }
  } else if (not is_editable() and this->lookup_index) {
    // A non-editable combo picks the item its type-ahead letters matched
    // (Swing's BasicComboBoxUI selects the match of each typed letter).
    commit_selection(this->lookup_index);
  }
}

bool ComboBox::field_hit(int x) const {
  auto metrics = screen.get_text_metrics();
  auto insets = get_insets();
  auto margin = laf::LookAndFeel::get<Insets>(this, "ComboBox.margin", Insets { 0, 1, 0, 1 });
  auto cell = metrics->get_char_width(char32_t('M'));
  auto field_limit = get_width() - insets.right - margin.right - (ARROW_GAP_COLUMNS + ARROW_GLYPH_COLUMNS) * cell;
  return x < field_limit;
}

// ---- user actions ----------------------------------------------------------

void ComboBox::begin_editing_if_needed() {
  if (not this->is_editing) {
    if (this->caret_moved) {
      // The user moved the caret first: keep the text she is editing (the
      // selection or the committed custom value).
      this->draft = to_u32(display_text());
      this->draft_caret = this->draft.size();
    } else {
      // A fresh edit session: the first typed letter replaces the field's
      // text (the type-to-look-up gesture).
      this->draft.clear();
      this->draft_caret = 0;
    }
    this->is_editing = true;
    this->caret_moved = false;
    this->lookup_index = std::nullopt;
  }
}

void ComboBox::commit_selection(std::optional<size_t> index) {
  if (index and *index >= this->model->get_size()) {
    return;
  }

  // The action announces what the field shows after the commit: the picked
  // item's text, or the custom typed text when no item was picked.
  auto text = index ? this->model->get_item_at(*index) : display_text();

  this->is_editing = false;
  this->draft.clear();
  this->draft_caret = 0;
  this->caret_moved = false;
  this->lookup_index = std::nullopt;

  // A pick replaces the committed custom value; a custom-value commit keeps
  // the typed text in the field (the index model cannot hold it, so it stays
  // unselected but visible, the way Swing's editable combo keeps the editor's
  // text after a custom commit).
  if (index) {
    this->committed_text.clear();
  } else {
    this->committed_text = to_u32(text);
  }

  // Selecting through the model fires the model's ChangeEvent (which also
  // repaints); the combo's own ActionEvent announces the user's pick, as
  // Swing's JComboBox does.
  auto changed = this->model->get_selected_index() != index;
  this->model->set_selected_index(index);
  hide_popup(false);

  if (changed) {
    // The model event already handled the field and the preferred size.
    update_field_from_model();
  } else {
    // The selection did not move; the field must still show the committed
    // text again and the width may have changed (the draft is gone).
    update_field_from_model();
    update_preferred_size();
    repaint();
  }
  fire_action_event(text);
}

void ComboBox::fire_action_event(std::string const &text) {
  auto modifiers = InputEvent::Modifiers { };
  auto when = screen.get_event_queue().get_most_recent_event_time();
  if (auto current = screen.get_event_queue().get_current_event()) {
    if (auto input_event = std::dynamic_pointer_cast<InputEvent>(current)) {
      modifiers = input_event->modifiers;
    } else if (auto action_event = std::dynamic_pointer_cast<ActionEvent>(current)) {
      modifiers = action_event->modifiers;
    }
  }
  fire_event<ActionEvent>(shared_from_this(), text, modifiers, when);
}

// ---- popup navigation --------------------------------------------------------

void ComboBox::popup_menu_wheel_moved(MouseWheelEvent &e) {
  if (not this->popup_visible) {
    return;
  }
  if (e.wheel_rotation != 0) {
    auto count = this->model->get_size();
    auto rows = std::min<size_t>(count, size_t(this->maximum_row_count));
    if (count > rows) {
      // One notch scrolls three rows, Swing's default units-to-scroll for a
      // list. The sign follows the scroll pane's wheel: a positive rotation
      // moves the window toward the later items.
      auto delta = 3 * (e.wheel_rotation < 0 ? -1 : 1);
      auto max_start = count - rows;
      auto target = std::clamp(static_cast<long long>(this->popup_scroll_start) + delta, 0LL, static_cast<long long>(max_start));
      if (size_t(target) != this->popup_scroll_start) {
        this->popup_scroll_start = size_t(target);
        // The armed row (the keyboard selection) stays where it is: it is
        // highlighted again only when the scroll lands back on it, exactly
        // like scrolling a list whose selection scrolled out of view.
        rebuild_popup_rows();
      }
    }
  }
  e.consume();
}

void ComboBox::move_popup_selection(int direction) {
  auto count = this->model->get_size();
  if (count == 0) {
    return;
  }
  auto current = this->popup_armed_index.value_or(this->model->get_selected_index().value_or(0));
  auto next = (current + count + direction) % count;
  this->popup_armed_index = next;
  ensure_popup_armed_visible();
  rebuild_popup_rows();
}

void ComboBox::page_popup_selection(int direction) {
  if (not this->popup_armed_index) {
    move_popup_selection(direction);
    return;
  }
  auto rows = size_t(this->maximum_row_count);
  auto count = this->model->get_size();
  auto target = direction > 0 ? std::min(*this->popup_armed_index + rows, count - 1) : (*this->popup_armed_index >= rows ? *this->popup_armed_index - rows : 0);
  this->popup_armed_index = target;
  ensure_popup_armed_visible();
  rebuild_popup_rows();
}

void ComboBox::jump_popup_selection(bool to_end) {
  if (this->model->get_size() == 0) {
    return;
  }
  this->popup_armed_index = to_end ? this->model->get_size() - 1 : 0;
  ensure_popup_armed_visible();
  rebuild_popup_rows();
}

void ComboBox::ensure_popup_armed_visible() {
  if (not this->popup_armed_index) {
    return;
  }
  auto count = this->model->get_size();
  auto rows = std::min<size_t>(count, size_t(this->maximum_row_count));
  if (rows == 0) {
    return;
  }
  auto armed = *this->popup_armed_index;
  if (armed < this->popup_scroll_start) {
    this->popup_scroll_start = armed;
  } else if (armed >= this->popup_scroll_start + rows) {
    this->popup_scroll_start = armed - rows + 1;
  }
}

// ---- keyboard ---------------------------------------------------------------

// Applies a draft-editing key (Backspace, Delete, the caret arrows, Home,
// End) to the editable field's draft and re-runs the lookup, so the first
// item matching the draft is re-highlighted after every change -- a live
// lookup. With the dropdown open these keys edit the draft too (only Home
// and End stay with the popup selection there); without one they move the
// caret of an edit in progress.
void ComboBox::handle_draft_edit_key(KeyEvent::KeyCode code) {
  switch (code) {
  case KeyEvent::VK_LEFT:
    this->caret_moved = true;
    begin_editing_if_needed();
    if (this->draft_caret != 0) {
      this->draft_caret -= 1;
      repaint();
    }
    break;
  case KeyEvent::VK_RIGHT:
    this->caret_moved = true;
    begin_editing_if_needed();
    if (this->draft_caret < this->draft.size()) {
      this->draft_caret += 1;
      repaint();
    }
    break;
  case KeyEvent::VK_HOME:
    this->caret_moved = true;
    begin_editing_if_needed();
    this->draft_caret = 0;
    repaint();
    break;
  case KeyEvent::VK_END:
    this->caret_moved = true;
    begin_editing_if_needed();
    this->draft_caret = this->draft.size();
    repaint();
    break;
  case KeyEvent::VK_BACK_SPACE:
    begin_editing_if_needed();
    if (this->draft_caret != 0) {
      this->draft.erase(this->draft.begin() + (this->draft_caret - 1));
      this->draft_caret -= 1;
      set_lookup_prefix(this->draft);
      repaint();
    }
    break;
  case KeyEvent::VK_DELETE:
    begin_editing_if_needed();
    if (this->draft_caret < this->draft.size()) {
      this->draft.erase(this->draft.begin() + this->draft_caret);
      set_lookup_prefix(this->draft);
      repaint();
    }
    break;
  default:
    break;
  }
}

void ComboBox::on_key_pressed(KeyEvent &e) {
  auto code = e.get_key_code();

  // The draft-editing keys of an editable combo work whether the dropdown is
  // open or not -- Backspace/Delete correct the lookup text and the caret
  // arrows move through it while the first matching item is highlighted live.
  if (is_editable() and code != KeyEvent::VK_HOME and code != KeyEvent::VK_END) {
    switch (code) {
    case KeyEvent::VK_LEFT:
    case KeyEvent::VK_RIGHT:
    case KeyEvent::VK_BACK_SPACE:
    case KeyEvent::VK_DELETE:
      handle_draft_edit_key(code);
      e.consume();
      return;
    default:
      break;
    }
  }

  if (this->popup_visible) {
    switch (code) {
    case KeyEvent::VK_UP:
      move_popup_selection(-1);
      e.consume();
      return;
    case KeyEvent::VK_DOWN:
      move_popup_selection(+1);
      e.consume();
      return;
    case KeyEvent::VK_PAGE_UP:
      page_popup_selection(-1);
      e.consume();
      return;
    case KeyEvent::VK_PAGE_DOWN:
      page_popup_selection(+1);
      e.consume();
      return;
    case KeyEvent::VK_HOME:
      jump_popup_selection(false);
      e.consume();
      return;
    case KeyEvent::VK_END:
      jump_popup_selection(true);
      e.consume();
      return;
    case KeyEvent::VK_ENTER: {
      auto index = this->popup_armed_index;
      if (not index and this->lookup_index) {
        index = this->lookup_index;
      }
      if (index) {
        commit_selection(index);
      } else if (is_editable() and this->is_editing) {
        // Enter with an open dropdown, no row highlighted and a draft in
        // flight commits the field's text as the custom value.
        commit_selection(std::nullopt);
      } else {
        // Nothing to pick or commit: Enter simply dismisses the dropdown. It
        // never touches the field or the selection, so a stray or repeated
        // Enter after a commit cannot discard them.
        hide_popup(false);
      }
      e.consume();
      return;
    }
    case KeyEvent::VK_ESCAPE:
      // Cancel the popup and any uncommitted edit, as Swing's Escape does.
      hide_popup(true);
      e.consume();
      return;
    default:
      break;
    }
  } else if (code == KeyEvent::VK_UP or code == KeyEvent::VK_DOWN) {
    // The arrow keys open the dropdown (editable and non-editable alike), as
    // Swing's combo does.
    if (is_editable()) {
      begin_editing_if_needed();
    }
    show_popup();
    e.consume();
    return;
  } else if (is_editable()) {
    switch (code) {
    case KeyEvent::VK_ENTER: {
      // Commit: the item the typed text matched -- or, when nothing matches
      // and a draft is in flight, keep the typed text as the combo's custom
      // value (it stays in the field). Enter with nothing to commit -- no
      // match, no draft -- is a no-op, so a repeated Enter after a commit
      // cannot wipe the selection or the field.
      if (auto index = this->lookup_index) {
        commit_selection(index);
      } else if (this->is_editing) {
        commit_selection(std::nullopt);
      }
      e.consume();
      return;
    }
    case KeyEvent::VK_ESCAPE:
      // Revert an uncommitted edit.
      if (this->is_editing) {
        this->is_editing = false;
        this->draft.clear();
        this->draft_caret = 0;
        this->caret_moved = false;
        this->lookup_index = std::nullopt;
        repaint();
      }
      e.consume();
      return;
    case KeyEvent::VK_HOME:
    case KeyEvent::VK_END:
      // Home/End without an open dropdown move the caret; with the dropdown
      // open they belong to the popup selection (see above).
      handle_draft_edit_key(code);
      e.consume();
      return;
    default:
      break;
    }
  } else {
    // A non-editable combo's type-ahead buffer expires after a delay, like
    // Swing's KeySelectionManager.
    auto now = EventClock::now();
    if (now - this->lookup_buffer_time > TYPE_AHEAD_DELAY) {
      this->lookup_buffer.clear();
    }
  }
}

void ComboBox::on_key_typed(KeyEvent &e) {
  if (not is_printable(e)) {
    return;
  }
  auto code = e.get_key_char().get_code();
  if (is_editable()) {
    // "Lookup editing": the letter goes into the draft, and the dropdown
    // (when open) follows the typed prefix; the match is committed by Enter.
    begin_editing_if_needed();
    this->caret_moved = true;
    this->draft.insert(this->draft.begin() + this->draft_caret, code);
    this->draft_caret += 1;
    set_lookup_prefix(this->draft);
    repaint();
    e.consume();
  } else {
    // Type-ahead lookup on a non-editable combo. Letters that stop matching
    // drop the buffer's first letter until a match is found again (a sliding
    // window, as Swing's KeySelectionManager searches).
    this->lookup_buffer.push_back(code);
    if (this->popup_visible) {
      set_lookup_prefix(std::u32string { char32_t(code) });
    } else {
      while (not this->lookup_buffer.empty()) {
        set_lookup_prefix(this->lookup_buffer);
        if (this->lookup_index) {
          break;
        }
        this->lookup_buffer.erase(this->lookup_buffer.begin());
      }
    }
    this->lookup_buffer_time = EventClock::now();
    e.consume();
  }
}

// ---- mouse ---------------------------------------------------------------

void ComboBox::on_mouse_pressed(MousePressEvent &e) {
  if (e.id != MousePressEvent::MOUSE_PRESSED) {
    return;
  }

  request_focus(false, true, FocusEvent::Cause::ACTIVATION);

  if (not field_hit(e.x) or not is_editable()) {
    // The arrow of an editable combo, and every press on a non-editable one
    // (its whole face is the button, as in Swing), toggles the dropdown.
    set_popup_visible(not this->popup_visible);
    e.consume();
    return;
  }

  // A press on the field of an editable combo places the caret and keeps (or
  // starts) an edit session; the dropdown stays as it is.
  this->caret_moved = true;
  begin_editing_if_needed();
  auto metrics = screen.get_text_metrics();
  auto insets = get_insets();
  auto margin = laf::LookAndFeel::get<Insets>(this, "ComboBox.margin", Insets { 0, 1, 0, 1 });
  auto x = insets.left + margin.left;
  auto caret = size_t { 0 };
  for (auto code : this->draft) {
    auto width = metrics->get_char_width(code);
    if (e.x < x + width / 2) {
      break;
    }
    x += width;
    caret += 1;
  }
  this->draft_caret = caret;
  repaint();
  e.consume();
}

void ComboBox::on_focus_changed(bool gained) {
  if (gained) {
    repaint();
  } else {
    // Losing the focus closes the dropdown. An uncommitted draft survives so
    // the user can come back to it (Swing's editor keeps its text); Escape
    // reverts it explicitly.
    hide_popup(false);
  }
}

// ---- preferred size and painting ---------------------------------------------

void ComboBox::update_preferred_size() {
  // The preferred width shows the widest item plus the arrow; the height is
  // one line (Swing's JComboBox preferred size follows the widest item).
  auto metrics = screen.get_text_metrics();
  auto margin = laf::LookAndFeel::get<Insets>(this, "ComboBox.margin", Insets { 0, 1, 0, 1 });
  auto insets = get_insets();
  auto cell = metrics->get_char_width(char32_t('M'));

  auto widest = 0;
  for (auto i = size_t { 0 }, n = this->model->get_size(); i != n; ++i) {
    widest = std::max(widest, metrics->get_width(this->model->get_item_at(i)));
  }
  if (this->is_editing) {
    widest = std::max(widest, metrics->get_width(draft_utf8()));
  } else if (not this->model->get_selected_index()) {
    // The field shows the committed custom value; make room for it.
    widest = std::max(widest, metrics->get_width(to_utf8(this->committed_text)));
  }

  auto arrow = (ARROW_GAP_COLUMNS + ARROW_GLYPH_COLUMNS) * cell;
  auto width = insets.left + margin.left + widest + arrow + margin.right + insets.right;
  auto height = insets.top + margin.top + metrics->get_line_height() + margin.bottom + insets.bottom;
  set_preferred_size(Dimension { width, height });
}

void ComboBox::paint(Graphics &g) {
  auto metrics = screen.get_text_metrics();
  auto margin = laf::LookAndFeel::get<Insets>(this, "ComboBox.margin", Insets { 0, 1, 0, 1 });
  auto insets = get_insets();
  auto cell = metrics->get_char_width(char32_t('M'));

  auto background = get_background_color().value_or(Color { 0xC0, 0xC0, 0xC0 });
  auto foreground = get_foreground_color().value_or(Color { 0, 0, 0 });
  g.set_background_color(background);
  g.set_foreground_color(foreground);
  g.fill_rect(0, 0, get_width(), get_height());

  auto x0 = insets.left + margin.left;
  auto y0 = insets.top + margin.top;
  auto x1 = get_width() - insets.right - margin.right;
  auto y1 = get_height() - insets.bottom - margin.bottom;
  if (x1 <= x0 or y1 <= y0) {
    return;
  }

  // The arrow strip sits at the right edge; the field ends before it.
  auto arrow_area = (ARROW_GAP_COLUMNS + ARROW_GLYPH_COLUMNS) * cell;
  auto arrow_x = x1 - cell;
  auto field_limit = x1 - arrow_area;

  auto text = display_text();
  auto y = y0 + std::max(0, (y1 - y0 - metrics->get_line_height()) / 2);

  // Cell of the caret (an editable, focused combo): the sum of the glyph
  // widths before the caret.
  auto show_caret = is_editable() and is_focus_owner();
  auto caret_x = x0;
  auto caret_glyph = Char(' ');
  if (show_caret) {
    auto caret_index = std::min(this->draft_caret, this->draft.size());
    for (auto i = size_t { 0 }; i != caret_index; ++i) {
      caret_x += metrics->get_char_width(this->draft[i]);
    }
    if (caret_index < this->draft.size()) {
      caret_glyph = Char(this->draft[caret_index]);
    }
    if (caret_x >= field_limit) {
      show_caret = false;
    }
  }

  // The field text, clipped to the field; the caret glyph is drawn after the
  // text with the field colors swapped (a solid block on both backends).
  auto x = x0;
  auto pos = size_t { 0 };
  for (auto it = to_chars(text), last = tui::end(it); it != last; ++it) {
    auto glyph = *it;
    auto width = metrics->get_char_width(glyph.get_code());
    if (x + width > field_limit) {
      break;
    }
    g.draw_char(glyph, x, y);
    x += width;
    pos += 1;
  }

  if (show_caret and caret_x + metrics->get_char_width(caret_glyph.get_code()) <= field_limit) {
    g.set_background_color(foreground);
    g.set_foreground_color(background);
    g.draw_char(caret_glyph, caret_x, y);
    g.set_background_color(background);
    g.set_foreground_color(foreground);
  }

  // The arrow reads "pressed" while the dropdown is open.
  if (this->popup_visible) {
    g.set_background_color(foreground);
    g.set_foreground_color(background);
    g.draw_char(Symbols::TRIANGLE_DOWN_POINTING_BLACK, arrow_x, y);
    g.set_background_color(background);
    g.set_foreground_color(foreground);
  } else {
    g.draw_char(Symbols::TRIANGLE_DOWN_POINTING_BLACK, arrow_x, y);
  }
}

}
