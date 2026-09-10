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
#include <tui++/TextField.h>
#include <tui++/Window.h>

#include <tui++/event/FocusEvent.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/util/utf-8.h>

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

    // A press on the combo or one of its children (the editor field) leaves
    // the popup alone: the combo's own handler toggles it, and a press in the
    // editor only moves the caret. A press inside the dropdown's window is a
    // row pick or a wheel-free scroll; everything else dismisses the popup.
    for (auto c = source; c; c = c->get_parent()) {
      if (c.get() == combo.get()) {
        return;
      }
    }
    if (source->get_containing_window() == combo->popup_window()) {
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

  // The editor field of an editable combo (Swing's BasicComboBoxEditor): a
  // TextField child laid out over the field area. It stays hidden until
  // set_editable(true). The combo keeps the keyboard focus and forwards its
  // keys to the editor; the editor is not focusable itself, so a press in it
  // focuses the combo (Swing's BasicComboBoxUI does the same with its editor).
  this->editor = make_component<TextField>();
  this->editor->set_name("combo editor");
  this->editor->set_focusable(false);
  this->editor->set_editable(false);
  this->editor->set_visible(false);
  this->editor->add_listener([this](ChangeEvent &) {
    editor_changed();
  });
  add(this->editor);

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

  set_editor_text(selected_item_text());
  update_preferred_size();
  layout_editor();
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
  this->lookup_index = std::nullopt;
  set_popup_visible(false);

  set_editor_text(selected_item_text());
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
    // Dropping the editable mode discards the edit in flight and the custom
    // value; enabling it starts from the current selection's text and lays the
    // editor over the field area.
    this->lookup_index = std::nullopt;
    if (this->editor) {
      this->editor->set_editable(value);
      this->editor->set_visible(value);
      this->editor->set_editing_focus(value and is_focus_owner());
    }
    if (value) {
      set_editor_text(selected_item_text());
    }
    update_preferred_size();
    layout_editor();
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
  // The editor owns the field's text while the combo is editable: the selected
  // item, a committed custom value and an edit in flight all live there.
  if (this->editable and this->editor) {
    return this->editor->get_text();
  }
  return selected_item_text();
}

void ComboBox::set_editor_text(std::string const &text) {
  if (not this->editor) {
    return;
  }
  // Programmatic updates must not read as user edits (they would run the
  // lookup and the commit tests against the combo's own text).
  this->setting_editor_text = true;
  this->editor->set_text(text);
  this->setting_editor_text = false;
  this->editor_baseline = text;
  // The combo's value arrives selected, so the first typed letter replaces it
  // (the type-to-look-up gesture); a click or an arrow key inside the field
  // collapses the selection and the next letter inserts at the caret, as the
  // old draft's "caret moved" rule did.
  this->editor->select_all();
}

bool ComboBox::editor_modified() const {
  return this->editor and this->editor->get_text() != this->editor_baseline;
}

void ComboBox::editor_changed() {
  if (this->setting_editor_text or not this->editor) {
    return;
  }
  // The user edited the field: the first item matching its text is
  // highlighted live (when the dropdown is open, and by the arrows when a
  // non-editable combo looks items up).
  set_lookup_prefix(util::to_u32(this->editor->get_text()));
  update_preferred_size();
}

void ComboBox::model_changed(ChangeEvent &e) {
  // Contents and selection changes come through the model. An external change
  // (set_selected_index from the application, model edits) supersedes an edit
  // in flight, the way Swing's setSelectedItem rewrites the editor's text.
  this->lookup_index = std::nullopt;

  if (this->popup_visible) {
    this->popup_armed_index = std::nullopt;
    rebuild_popup_rows();
  }
  set_editor_text(selected_item_text());
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

void ComboBox::hide_popup(bool revert_edit) {
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
  if (revert_edit) {
    // Escape: cancel the edit in flight and restore the combo's own value
    // (the selected item, or the committed custom text).
    this->lookup_index = std::nullopt;
    set_editor_text(this->editor_baseline);
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
    auto item = util::to_u32(this->model->get_item_at(i));
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

void ComboBox::commit_selection(std::optional<size_t> index) {
  if (index and *index >= this->model->get_size()) {
    return;
  }

  // The action announces what the field shows after the commit: the picked
  // item's text, or the custom typed text when no item was picked.
  auto text = index ? this->model->get_item_at(*index) : display_text();

  this->lookup_index = std::nullopt;

  // Selecting through the model fires the model's ChangeEvent (which also
  // repaints); the combo's own ActionEvent announces the user's pick, as
  // Swing's JComboBox does.
  this->model->set_selected_index(index);
  hide_popup(false);

  // A pick puts the item's text into the editor; a custom-value commit keeps
  // the typed text (the index model cannot hold it, so it stays unselected
  // but visible, the way Swing's editable combo keeps the editor's text after
  // a custom commit). Either way the combo's value is now the editor's text:
  // the re-assert also covers the model's ChangeEvent, which wrote the empty
  // selection into the editor on a custom commit.
  set_editor_text(index ? this->model->get_item_at(*index) : text);
  update_preferred_size();
  repaint();
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

// The combo's keys, split between the popup, the editor and the combo (Swing's
// JComboBox and its editor split them the same way): Up/Down/Page/Home/End
// drive the open dropdown, Enter picks or commits and Escape cancels; every
// other key of an editable combo belongs to the editor field -- the caret and
// selection commands, the deletions, the clipboard chords, and Home/End while
// the dropdown is closed. The editor's ChangeEvent drives the live lookup.
void ComboBox::on_key_pressed(KeyEvent &e) {
  auto code = e.get_key_code();

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
      } else if (is_editable() and editor_modified()) {
        // Enter with an open dropdown, no row highlighted and an edit in
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
    show_popup();
    e.consume();
    return;
  } else if (is_editable()) {
    switch (code) {
    case KeyEvent::VK_ENTER: {
      // Commit: the item the typed text matched -- or, when nothing matches
      // and the field was edited, keep the typed text as the combo's custom
      // value (it stays in the field). Enter with nothing to commit -- no
      // match, no edit -- is a no-op, so a repeated Enter after a commit
      // cannot wipe the selection or the field.
      if (auto index = this->lookup_index) {
        commit_selection(index);
      } else if (editor_modified()) {
        commit_selection(std::nullopt);
      }
      e.consume();
      return;
    }
    case KeyEvent::VK_ESCAPE:
      // Revert an uncommitted edit.
      if (editor_modified()) {
        this->lookup_index = std::nullopt;
        set_editor_text(this->editor_baseline);
      }
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

  // Everything else belongs to the editor of an editable combo: the caret and
  // selection commands, the deletions, the clipboard chords and Home/End
  // (when the dropdown is closed; with it open they drive the popup above).
  // The editor's ChangeEvent drives the live lookup.
  if (is_editable() and this->editor) {
    this->editor->on_key_pressed(e);
  }
}

void ComboBox::on_key_typed(KeyEvent &e) {
  if (is_editable()) {
    // "Lookup editing": the character goes into the editor, and the dropdown
    // (when open) follows the typed prefix; the match is committed by Enter.
    if (this->editor) {
      this->editor->on_key_typed(e);
    }
    return;
  }

  if (not is_printable(e)) {
    return;
  }
  auto code = e.get_key_char().get_code();

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

// ---- mouse ---------------------------------------------------------------

void ComboBox::on_mouse_pressed(MousePressEvent &e) {
  if (e.id != MousePressEvent::MOUSE_PRESSED) {
    return;
  }

  // The popup trigger is the context menu's gesture: it must not toggle the
  // dropdown underneath the menu that is about to open.
  if (e.is_popup_trigger and get_component_popup_menu()) {
    return;
  }

  request_focus(false, true, FocusEvent::Cause::ACTIVATION);

  // The editor child handles the presses on the field itself (the caret and
  // the selection gestures). What reaches the combo is the arrow strip of an
  // editable combo, and the whole face of a non-editable one (its face is the
  // button, as in Swing): both toggle the dropdown.
  if (not is_editable() or not field_hit(e.x)) {
    set_popup_visible(not this->popup_visible);
  }
  e.consume();
}

void ComboBox::on_focus_changed(bool gained) {
  if (this->editor) {
    this->editor->set_editing_focus(gained and is_editable());
  }
  if (gained) {
    // An editable combo selects the editor's text when it takes the focus, so
    // the first typed letter replaces the field's value instead of appending
    // to it (the type-to-look-up gesture, as Swing's editor behaves).
    if (is_editable() and this->editor) {
      this->editor->select_all();
    }
    repaint();
  } else {
    // Losing the focus closes the dropdown. An uncommitted edit survives so
    // the user can come back to it (Swing's editor keeps its text); Escape
    // reverts it explicitly.
    hide_popup(false);
  }
}

// ---- layout, preferred size and painting --------------------------------------

void ComboBox::do_layout() {
  base::do_layout();
  layout_editor();
}

void ComboBox::layout_editor() {
  if (not this->editor) {
    return;
  }
  // The editor covers the field area: from the combo's border to the arrow
  // strip. Its own "TextField.margin" insets give the text the same one-cell
  // inset the combo's margin does; a press anywhere in it belongs to the
  // editor (caret placement and selection).
  auto metrics = screen.get_text_metrics();
  auto margin = laf::LookAndFeel::get<Insets>(this, "ComboBox.margin", Insets { 0, 1, 0, 1 });
  auto insets = get_insets();
  auto cell = metrics->get_char_width(char32_t('M'));
  auto arrow_area = (ARROW_GAP_COLUMNS + ARROW_GLYPH_COLUMNS) * cell;
  auto bounds = Rectangle { insets.left, insets.top, get_width() - insets.left - insets.right - margin.right - arrow_area, get_height() - insets.top - insets.bottom };
  if (bounds.width <= 0 or bounds.height <= 0) {
    return;
  }
  if (this->editor->get_bounds() != bounds) {
    this->editor->set_bounds(bounds.x, bounds.y, bounds.width, bounds.height);
  }
}

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
  if (this->editor) {
    // The field may hold more than any item: an edit in flight, or a
    // committed custom value.
    widest = std::max(widest, metrics->get_width(this->editor->get_text()));
  }

  auto arrow = (ARROW_GAP_COLUMNS + ARROW_GLYPH_COLUMNS) * cell;
  auto width = insets.left + margin.left + widest + arrow + margin.right + insets.right;
  auto height = insets.top + margin.top + metrics->get_line_height() + margin.bottom + insets.bottom;
  set_preferred_size(Dimension { width, height });
}

void ComboBox::paint_component(Graphics &g) {
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

  // The editor child paints the field's text, its selection and its caret
  // (Swing's BasicComboBoxUI paints the renderer itself only when there is no
  // editor); the combo paints the item text for a non-editable combo.
  auto y = y0 + std::max(0, (y1 - y0 - metrics->get_line_height()) / 2);
  if (not is_editable()) {
    auto text = display_text();
    auto x = x0;
    for (auto it = to_chars(text), last = tui::end(it); it != last; ++it) {
      auto glyph = *it;
      auto width = metrics->get_char_width(glyph.get_code());
      if (x + width > field_limit) {
        break;
      }
      g.draw_char(glyph, x, y);
      x += width;
    }
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
