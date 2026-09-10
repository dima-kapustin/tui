#include <tui++/TextField.h>

#include <tui++/Clipboard.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>
#include <tui++/Window.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/util/utf-8.h>

#include <algorithm>

namespace tui {

namespace {

// The DefaultCaret's default blink rate (Swing: 500 ms; TextArea uses 530).
constexpr auto DEFAULT_BLINK_RATE = std::chrono::milliseconds { 530 };

// The most undoable steps a field keeps; each one is a whole-line snapshot.
constexpr size_t MAX_UNDO_STEPS = 100;

// A character the field accepts from typing: plain (optionally shifted)
// printable characters; Ctrl/Alt/Meta chords are commands, not content.
bool is_printable(KeyEvent const &e) {
  if (e.modifiers & (InputEvent::CTRL_DOWN | InputEvent::ALT_DOWN | InputEvent::META_DOWN)) {
    return false;
  }
  auto code = e.get_key_char().get_code();
  return code >= 0x20 and code != 0x7f;
}

// The paste of a multi-line text into a single-line field: Swing, SWT and the
// native edits all flatten the line breaks; a space keeps the run's words
// apart, the way a "paste as text" of a wrapped paragraph reads. A CRLF pair
// collapses into its single space.
std::u32string single_line(std::u32string text) {
  auto result = std::u32string { };
  result.reserve(text.size());
  for (auto i = std::size_t { 0 }; i < text.size(); ++i) {
    auto code = text[i];
    if (code == '\r') {
      if (i + 1 < text.size() and text[i + 1] == '\n') {
        ++i;
      }
      result.push_back(' ');
    } else if (code == '\n') {
      result.push_back(' ');
    } else {
      result.push_back(code);
    }
  }
  return result;
}

} // namespace

// The framework dispatches key events to windows, not to the focus owner, so
// the field registers a forwarder on its owning window while it is
// displayable. Defined outside the anonymous namespace so the friend
// declaration in the header (tui::TextFieldKeyForwarder) names the same class.
class TextFieldKeyForwarder final: public EventListener<KeyEvent> {
  std::weak_ptr<TextField> field;

public:
  explicit TextFieldKeyForwarder(std::weak_ptr<TextField> const &field) :
      field(field) {
  }

  virtual void key_pressed(KeyEvent &e) override {
    if (not e.consumed) {
      if (auto field = this->field.lock(); field and field->is_focus_owner()) {
        field->on_key_pressed(e);
      }
    }
  }

  virtual void key_typed(KeyEvent &e) override {
    if (not e.consumed) {
      if (auto field = this->field.lock(); field and field->is_focus_owner()) {
        field->on_key_typed(e);
      }
    }
  }
};

// Drags (and the release that ends them) are retargeted to the component that
// received the press, but a Component dispatches them only to screen
// listeners; see WindowMouseEventDispatcher and TextAreaDragObserver. This
// observer is registered on the screen while the left button is down on the
// field and extends the selection to every dragged-over cell.
class TextFieldDragObserver final: public EventListener<Event>, public std::enable_shared_from_this<TextFieldDragObserver> {
  std::weak_ptr<TextField> field;

public:
  explicit TextFieldDragObserver(std::weak_ptr<TextField> const &field) :
      field(field) {
  }

  virtual void event_dispatched(Event &e) override {
    if (auto field = this->field.lock()) {
      auto from_field = std::dynamic_pointer_cast<Component>(e.source) == field;
      if (e.id == MouseDragEvent::MOUSE_DRAGGED) {
        if (from_field) {
          field->on_mouse_drag(static_cast<MouseDragEvent&>(e).x);
        }
      } else if (e.id == MousePressEvent::MOUSE_RELEASED and from_field) {
        // The release ends the drag; drop the observer (the next press
        // registers it again).
        field->on_mouse_release();
        field->unregister_drag_observer();
      }
      return;
    }
    // The field died while being dragged (its window closed): stop observing.
    screen.remove_listener(shared_from_this());
  }
};

// ---- construction ----------------------------------------------------------

TextField::TextField() {
  set_name("text field");
}

TextField::TextField(std::string const &text) {
  set_name("text field");
  this->text = util::to_u32(text);
  // The caret starts at the document's beginning, as it does for a Swing
  // field constructed with text (setText's contract).
}

TextField::TextField(int columns) :
    columns(columns) {
  set_name("text field");
}

TextField::TextField(std::string const &text, int columns) :
    columns(columns) {
  set_name("text field");
  this->text = util::to_u32(text);
}

TextField::~TextField() {
  this->blink_timer.stop();
}

void TextField::init() {
  base::init();

  // As in Swing's BasicTextFieldUI, the field paints the theme's text-field
  // colors ("TextField.BackgroundColor"/"ForegroundColor", installed from the
  // system window colors); a program can override either with
  // set_background_color / set_foreground_color.
  set_opaque(true);
  laf::LookAndFeel::install_colors(this, "TextField.BackgroundColor", "TextField.ForegroundColor");

  add_listener([this](FocusEvent &e) {
    focus_changed(e.id == FocusEvent::FOCUS_GAINED);
  });
  add_listener([this](MousePressEvent &e) {
    on_mouse_pressed(e);
  });
  add_listener(MouseClickEvent::MOUSE_CLICKED, [this](MouseClickEvent &e) {
    on_mouse_click(e);
  });

  update_preferred_size();
}

void TextField::add_notify() {
  if (auto window = get_containing_window()) {
    this->key_forwarder = std::make_shared<TextFieldKeyForwarder>(std::static_pointer_cast<TextField>(shared_from_this()));
    window->add_listener(this->key_forwarder);
  }
  base::add_notify();
}

void TextField::remove_notify() {
  unregister_drag_observer();
  if (auto window = get_containing_window(); window and this->key_forwarder) {
    window->remove_listener(this->key_forwarder);
  }
  this->key_forwarder.reset();
  base::remove_notify();
}

void TextField::request_input_focus() {
  request_focus(FocusEvent::Cause::ACTIVATION);
}

// ---- content ---------------------------------------------------------------

std::string TextField::get_text() const {
  return util::to_utf8(this->text);
}

void TextField::set_text(std::string const &text) {
  this->text = util::to_u32(text);
  // Swing's setText replaces the document and leaves the caret at its start
  // (the view shows the text from the beginning); a program that wants to
  // continue from elsewhere sets the caret explicitly. The undo history is
  // cleared, as Swing's setText is not an undoable edit.
  this->dot = this->mark = 0;
  this->undo_stack.clear();
  this->redo_stack.clear();
  restart_caret_blink();
  notify_change();
}

void TextField::set_columns(int columns) {
  if (this->columns != columns) {
    this->columns = columns;
    update_preferred_size();
  }
}

void TextField::set_editable(bool value) {
  if (this->editable != value) {
    this->editable = value;
    // A read-only field keeps its caret and its selection (Swing allows
    // selecting and copying from a non-editable text component); only the
    // editing commands refuse to change the content.
    repaint();
  }
}

void TextField::set_horizontal_alignment(HorizontalAlignment alignment) {
  if (this->horizontal_alignment != alignment) {
    this->horizontal_alignment = alignment;
    repaint();
  }
}

// ---- caret and selection ----------------------------------------------------

std::size_t TextField::get_selection_start() const {
  return std::min(this->dot, this->mark);
}

std::size_t TextField::get_selection_end() const {
  return std::max(this->dot, this->mark);
}

void TextField::set_caret_position(std::size_t position) {
  move_dot(position, false);
}

void TextField::move_caret_position(std::size_t position) {
  move_dot(position, true);
}

void TextField::select(std::size_t start, std::size_t end) {
  auto size = this->text.size();
  start = std::min(start, size);
  end = std::min(end, size);
  this->dot = end;
  this->mark = start;
  restart_caret_blink();
  repaint();
}

void TextField::select_all() {
  select(0, this->text.size());
}

void TextField::replace_selection(std::string const &text) {
  replace_range(get_selection_start(), get_selection_end(), util::to_u32(text));
}

void TextField::set_caret_visible(bool value) {
  if (this->caret_visible != value) {
    this->caret_visible = value;
    if (value) {
      restart_caret_blink();
    } else {
      this->blink_timer.stop();
      this->caret_on = true;
      repaint();
    }
  }
}

void TextField::set_caret_blink_rate(std::chrono::milliseconds rate) {
  if (this->caret_blink_rate != rate) {
    this->caret_blink_rate = rate;
    if (rate > std::chrono::milliseconds::zero() and is_focused_owner() and this->caret_visible) {
      this->blink_timer.set_period(rate);
      this->blink_timer.start();
    } else {
      this->blink_timer.stop();
      this->caret_on = true;
      repaint();
    }
  }
}

// ---- clipboard and undo ------------------------------------------------------

void TextField::copy() {
  if (not has_selection()) {
    return;
  }
  Clipboard::set_text(util::to_utf8(this->text.substr(get_selection_start(), get_selection_end() - get_selection_start())));
}

void TextField::cut() {
  if (not this->editable or not has_selection()) {
    return;
  }
  copy();
  replace_range(get_selection_start(), get_selection_end(), { });
}

void TextField::paste() {
  if (not this->editable or not Clipboard::has_text()) {
    return;
  }
  replace_range(get_selection_start(), get_selection_end(), single_line(util::to_u32(Clipboard::get_text())));
}

void TextField::undo() {
  if (this->undo_stack.empty()) {
    return;
  }
  this->redo_stack.push_back(Snapshot { this->text, this->dot, this->mark });
  auto snapshot = this->undo_stack.back();
  this->undo_stack.pop_back();
  restore(snapshot);
}

void TextField::redo() {
  if (this->redo_stack.empty()) {
    return;
  }
  this->undo_stack.push_back(Snapshot { this->text, this->dot, this->mark });
  auto snapshot = this->redo_stack.back();
  this->redo_stack.pop_back();
  restore(snapshot);
}

void TextField::post_action_event() {
  auto modifiers = InputEvent::Modifiers { };
  auto when = screen.get_event_queue().get_most_recent_event_time();
  if (auto current = screen.get_event_queue().get_current_event()) {
    if (auto input_event = std::dynamic_pointer_cast<InputEvent>(current)) {
      modifiers = input_event->modifiers;
    } else if (auto action_event = std::dynamic_pointer_cast<ActionEvent>(current)) {
      modifiers = action_event->modifiers;
    }
  }
  fire_event<ActionEvent>(shared_from_this(), util::to_utf8(this->text), modifiers, when);
}

void TextField::set_editing_focus(bool value) {
  if (this->editing_focus != value) {
    this->editing_focus = value;
    if (value) {
      restart_caret_blink();
    } else {
      this->blink_timer.stop();
      this->caret_on = true;
      repaint();
    }
  }
}

// ---- edits -------------------------------------------------------------------

void TextField::replace_range(std::size_t start, std::size_t end, std::u32string const &inserted) {
  auto size = this->text.size();
  start = std::min(start, size);
  end = std::min(end, size);
  if (start > end) {
    std::swap(start, end);
  }

  push_undo();
  this->text.replace(start, end - start, inserted);
  this->dot = this->mark = start + inserted.size();
  restart_caret_blink();
  notify_change();
}

void TextField::insert_typed(char32_t code) {
  replace_range(get_selection_start(), get_selection_end(), std::u32string { code });
}

void TextField::delete_backward(bool word) {
  if (has_selection()) {
    replace_range(get_selection_start(), get_selection_end(), { });
    return;
  }
  if (this->dot == 0) {
    return;
  }
  auto start = word ? previous_word_boundary(this->dot) : this->dot - 1;
  replace_range(start, this->dot, { });
}

void TextField::delete_forward(bool word) {
  if (has_selection()) {
    replace_range(get_selection_start(), get_selection_end(), { });
    return;
  }
  if (this->dot >= this->text.size()) {
    return;
  }
  auto end = word ? next_word_boundary(this->dot) : this->dot + 1;
  replace_range(this->dot, end, { });
}

void TextField::push_undo() {
  this->undo_stack.push_back(Snapshot { this->text, this->dot, this->mark });
  if (this->undo_stack.size() > MAX_UNDO_STEPS) {
    this->undo_stack.erase(this->undo_stack.begin());
  }
  this->redo_stack.clear();
}

void TextField::restore(Snapshot const &snapshot) {
  this->text = snapshot.text;
  this->dot = std::min(snapshot.dot, this->text.size());
  this->mark = std::min(snapshot.mark, this->text.size());
  restart_caret_blink();
  notify_change();
}

// ---- commands ------------------------------------------------------------------

void TextField::move_dot(std::size_t position, bool extend) {
  position = std::min(position, this->text.size());
  if (not extend) {
    this->mark = position;
  } else if (not has_selection()) {
    // The first Shift-move anchors the selection at the old caret.
    this->mark = this->dot;
  }
  this->dot = position;
  restart_caret_blink();
  repaint();
}

void TextField::move_dot_arrow(int direction, bool extend, bool word) {
  if (not extend and has_selection()) {
    // A plain arrow collapses the selection to its near edge instead of
    // stepping over it (the classic behavior of every text field).
    move_dot(direction < 0 ? get_selection_start() : get_selection_end(), false);
    return;
  }

  auto position = this->dot;
  if (direction < 0) {
    position = word ? previous_word_boundary(position) : (position == 0 ? 0 : position - 1);
  } else {
    position = word ? next_word_boundary(position) : std::min(position + 1, this->text.size());
  }
  move_dot(position, extend);
}

void TextField::move_dot_home(bool extend) {
  if (not extend and has_selection()) {
    move_dot(get_selection_start(), false);
    return;
  }
  move_dot(0, extend);
}

void TextField::move_dot_end(bool extend) {
  if (not extend and has_selection()) {
    move_dot(get_selection_end(), false);
    return;
  }
  move_dot(this->text.size(), extend);
}

// ---- word boundaries -----------------------------------------------------------

bool TextField::is_word_char(char32_t code) {
  return code == '_' //
      or (code >= '0' and code <= '9') //
      or (code >= 'A' and code <= 'Z') //
      or (code >= 'a' and code <= 'z') //
      or code >= 0x80; // letters of the other scripts count as word content
}

std::size_t TextField::previous_word_boundary(std::size_t from) const {
  auto i = std::min(from, this->text.size());
  while (i > 0 and not is_word_char(this->text[i - 1])) {
    --i;
  }
  while (i > 0 and is_word_char(this->text[i - 1])) {
    --i;
  }
  return i;
}

std::size_t TextField::next_word_boundary(std::size_t from) const {
  auto size = this->text.size();
  auto i = std::min(from, size);
  if (i < size and is_word_char(this->text[i])) {
    // Inside a word: its end is the next stop.
    while (i < size and is_word_char(this->text[i])) {
      ++i;
    }
  } else {
    // Between words: step over the separators, then past the next word.
    while (i < size and not is_word_char(this->text[i])) {
      ++i;
    }
    while (i < size and is_word_char(this->text[i])) {
      ++i;
    }
  }
  return i;
}

void TextField::select_word_at(std::size_t index) {
  auto size = this->text.size();
  if (size == 0) {
    return;
  }
  if (index >= size) {
    index = size - 1;
  }
  // The clicked cell's character decides: a word selects the word, a
  // separator selects the run of separators around it.
  auto word = is_word_char(this->text[index]);
  auto start = index;
  while (start > 0 and is_word_char(this->text[start - 1]) == word) {
    --start;
  }
  auto end = index + 1;
  while (end < size and is_word_char(this->text[end]) == word) {
    ++end;
  }
  select(start, end);
}

// ---- geometry -------------------------------------------------------------------

int TextField::content_left() const {
  auto insets = get_insets();
  auto margin = laf::LookAndFeel::get<Insets>(this, "TextField.margin", Insets { 0, 1, 0, 1 });
  return insets.left + margin.left;
}

int TextField::content_width() const {
  auto insets = get_insets();
  auto margin = laf::LookAndFeel::get<Insets>(this, "TextField.margin", Insets { 0, 1, 0, 1 });
  return get_width() - insets.left - margin.left - margin.right - insets.right;
}

int TextField::cells_before(std::size_t index) const {
  auto metrics = screen.get_text_metrics();
  auto cells = 0;
  for (auto i = std::size_t { 0 }; i < index and i < this->text.size(); ++i) {
    cells += metrics->get_char_width(this->text[i]);
  }
  return cells;
}

int TextField::view_offset() const {
  auto visible = content_width();
  if (visible <= 0) {
    return 0;
  }
  if (cells_before(this->text.size()) <= visible) {
    return 0; // the whole text fits: nothing to scroll
  }
  // Keep the caret visible: the caret's cell must lie inside the window, so a
  // caret past the right edge scrolls the text left by exactly that much.
  auto caret = cells_before(this->dot);
  if (caret >= visible) {
    return caret - visible + 1;
  }
  return 0;
}

int TextField::alignment_offset() const {
  auto visible = content_width();
  auto total = cells_before(this->text.size());
  if (visible <= 0 or total >= visible) {
    return 0; // alignment only applies while the text is narrower than the field
  }
  switch (this->horizontal_alignment) {
  case CENTER:
    return (visible - total) / 2;
  case TRAILING:
    return visible - total;
  default:
    return 0;
  }
}

std::size_t TextField::index_at(int x) const {
  auto metrics = screen.get_text_metrics();
  // The cell of the first glyph: the content origin, offset by the alignment
  // shift and the horizontal scroll (the inverse of the paint mapping).
  auto cell = content_left() + alignment_offset() - view_offset();
  for (auto i = std::size_t { 0 }; i < this->text.size(); ++i) {
    auto width = metrics->get_char_width(this->text[i]);
    if (x < cell + (width + 1) / 2) {
      return i; // the click is on the glyph's left half: the caret goes before it
    }
    cell += width;
  }
  return this->text.size();
}

// ---- state --------------------------------------------------------------------

void TextField::notify_change() {
  update_preferred_size();
  repaint();
  fire_event<ChangeEvent>(shared_from_this());
}

void TextField::update_preferred_size() {
  auto metrics = screen.get_text_metrics();
  auto insets = get_insets();
  auto margin = laf::LookAndFeel::get<Insets>(this, "TextField.margin", Insets { 0, 1, 0, 1 });
  auto cell = metrics->get_char_width(char32_t('M'));

  // Swing's columns set the preferred width; without them the field sizes to
  // its content (never smaller than one cell, so an empty field stays
  // visible).
  auto cells = std::max(this->columns > 0 ? this->columns : cells_before(this->text.size()), 1);
  auto width = insets.left + margin.left + cells * cell + margin.right + insets.right;
  auto height = insets.top + margin.top + metrics->get_line_height() + margin.bottom + insets.bottom;
  set_preferred_size(Dimension { width, height });
}

bool TextField::is_focused_owner() const {
  return is_focus_owner() or this->editing_focus;
}

bool TextField::is_caret_showing() const {
  return this->caret_visible and is_focused_owner() and (this->caret_blink_rate <= std::chrono::milliseconds::zero() or this->caret_on);
}

void TextField::restart_caret_blink() {
  this->caret_on = true;
  if (this->caret_blink_rate > std::chrono::milliseconds::zero() and is_focused_owner() and this->caret_visible) {
    this->blink_timer.start();
  }
  repaint();
}

void TextField::blink_tick() {
  if (is_focused_owner() and this->caret_visible and is_showing()) {
    this->caret_on = not this->caret_on;
    repaint();
  }
}

void TextField::focus_changed(bool gained) {
  if (gained) {
    restart_caret_blink();
  } else {
    this->blink_timer.stop();
    this->caret_on = true;
    repaint();
  }
}

// ---- painting -------------------------------------------------------------------

void TextField::paint(Graphics &g) {
  auto metrics = screen.get_text_metrics();
  auto insets = get_insets();
  auto margin = laf::LookAndFeel::get<Insets>(this, "TextField.margin", Insets { 0, 1, 0, 1 });

  auto background = get_background_color().value_or(Color { 0xFF, 0xFF, 0xFF });
  auto foreground = get_foreground_color().value_or(Color { 0, 0, 0 });
  auto selection_background = laf::LookAndFeel::get<std::optional<Color>>(this, "TextField.SelectionBackground", std::nullopt).value_or(Color { 0, 0, 0x80 });
  auto selection_foreground = laf::LookAndFeel::get<std::optional<Color>>(this, "TextField.SelectionForeground", std::nullopt).value_or(Color { 0xFF, 0xFF, 0xFF });

  g.set_background_color(background);
  g.set_foreground_color(foreground);
  g.fill_rect(0, 0, get_width(), get_height());

  auto width = content_width();
  if (width <= 0) {
    return;
  }

  auto left = content_left();
  auto right = left + width;
  auto y = insets.top + margin.top + std::max(0, (get_height() - insets.top - insets.bottom - margin.top - margin.bottom - metrics->get_line_height()) / 2);

  auto delta = left + alignment_offset() - view_offset();
  auto caret_cell = left + alignment_offset() + cells_before(this->dot) - view_offset();
  auto sel_start = get_selection_start();
  auto sel_end = get_selection_end();
  // An unfocused field keeps its selection internally (the host's next typed
  // letter still replaces it), but paints its text plain: only a focused text
  // component shows the selection highlight.
  auto show_selection = is_focused_owner();

  auto cell = delta;
  for (auto i = std::size_t { 0 }; i < this->text.size(); ++i) {
    auto glyph_width = metrics->get_char_width(this->text[i]);
    if (cell + glyph_width > left and cell < right) {
      if (show_selection and i >= sel_start and i < sel_end) {
        g.set_background_color(selection_background);
        g.set_foreground_color(selection_foreground);
      } else {
        g.set_background_color(background);
        g.set_foreground_color(foreground);
      }
      g.draw_char(Char(this->text[i]), cell, y);
    }
    cell += glyph_width;
  }

  // The caret, drawn after the text so the selection cannot hide it: the
  // glyph at the caret (a space past the last one) with the field's colors
  // swapped -- a solid block on both backends, like the combo box's editor.
  if (is_caret_showing() and caret_cell >= left and caret_cell < right) {
    auto caret_glyph = this->dot < this->text.size() ? Char(this->text[this->dot]) : Char(' ');
    g.set_background_color(foreground);
    g.set_foreground_color(background);
    g.draw_char(caret_glyph, caret_cell, y);
  }
}

// ---- mouse -----------------------------------------------------------------------

void TextField::on_mouse_pressed(MousePressEvent &e) {
  if (e.id == MousePressEvent::MOUSE_RELEASED) {
    // The release the drag observer missed (the pointer left the window, or
    // the observer already ended on a routed release).
    on_mouse_release();
    return;
  }
  if (e.id != MousePressEvent::MOUSE_PRESSED) {
    return;
  }

  // Focus: the field itself, or the host that keeps the focus for it (an
  // editable combo box, which forwards its keys here). The focus request runs
  // before the caret is placed: a host may select all on focus gain (the
  // type-to-replace gesture), and the click must override that.
  if (is_focusable()) {
    request_input_focus();
  } else if (auto host = get_parent()) {
    host->request_focus(FocusEvent::Cause::ACTIVATION);
  }

  auto shift = bool(e.modifiers & InputEvent::SHIFT_DOWN);
  auto index = index_at(e.x);
  if (shift and has_selection()) {
    // Shift+click extends the selection to the clicked spot (Swing's
    // selection-by-mouse protocol).
    move_dot(index, true);
  } else {
    move_dot(index, false);
  }

  // The press starts a drag selection: the drags that follow extend it from
  // the press point.
  this->mouse_dragging = true;
  register_drag_observer();
  e.consume();
}

void TextField::on_mouse_click(MouseClickEvent &e) {
  // The classic click gestures: a double-click selects the word, a
  // triple-click the whole line (which, in a single-line field, is all of it).
  if (e.click_count >= 3) {
    select_all();
    e.consume();
  } else if (e.click_count == 2) {
    select_word_at(index_at(e.x));
    e.consume();
  }
}

void TextField::on_mouse_drag(int x) {
  if (not this->mouse_dragging) {
    return;
  }
  move_dot(index_at(x), true);
}

void TextField::on_mouse_release() {
  this->mouse_dragging = false;
}

void TextField::register_drag_observer() {
  if (not this->drag_observer) {
    this->drag_observer = std::make_shared<TextFieldDragObserver>(std::static_pointer_cast<TextField>(shared_from_this()));
    screen.add_listener(EventType::MOUSE_DRAG | EventType::MOUSE_PRESS, this->drag_observer);
  }
}

void TextField::unregister_drag_observer() {
  if (this->drag_observer) {
    screen.remove_listener(this->drag_observer);
    this->drag_observer.reset();
  }
}

// ---- keyboard ---------------------------------------------------------------------

void TextField::on_key_pressed(KeyEvent &e) {
  auto code = e.get_key_code();
  auto ctrl = bool(e.modifiers & InputEvent::CTRL_DOWN);
  auto shift = bool(e.modifiers & InputEvent::SHIFT_DOWN);

  switch (code) {
  case KeyEvent::VK_LEFT:
    move_dot_arrow(-1, shift, ctrl);
    e.consume();
    break;
  case KeyEvent::VK_RIGHT:
    move_dot_arrow(+1, shift, ctrl);
    e.consume();
    break;
  case KeyEvent::VK_HOME:
    move_dot_home(shift);
    e.consume();
    break;
  case KeyEvent::VK_END:
    move_dot_end(shift);
    e.consume();
    break;
  case KeyEvent::VK_BACK_SPACE:
    if (this->editable) {
      delete_backward(ctrl);
    }
    e.consume();
    break;
  case KeyEvent::VK_DELETE:
    // Shift+Delete cuts (Swing's DefaultEditorKit binds the chord to
    // cut-to-clipboard); plain and Ctrl+Delete delete forward.
    if (shift and not ctrl) {
      cut();
    } else if (this->editable) {
      delete_forward(ctrl);
    }
    e.consume();
    break;
  case KeyEvent::VK_INSERT:
    // The Windows clipboard chords (Swing's JTextComponent binds the same):
    // Ctrl+Insert copies, Shift+Insert pastes.
    if (ctrl) {
      copy();
      e.consume();
    } else if (shift) {
      paste();
      e.consume();
    }
    break;
  case KeyEvent::VK_ENTER:
    // Swing's notify-field-accept: the field announces its content.
    post_action_event();
    e.consume();
    break;
  case KeyEvent::VK_A:
    if (ctrl) {
      select_all();
      e.consume();
    }
    break;
  case KeyEvent::VK_C:
    if (ctrl) {
      copy();
      e.consume();
    }
    break;
  case KeyEvent::VK_X:
    if (ctrl) {
      cut();
      e.consume();
    }
    break;
  case KeyEvent::VK_V:
    if (ctrl) {
      paste();
      e.consume();
    }
    break;
  case KeyEvent::VK_Z:
    if (ctrl) {
      shift ? redo() : undo();
      e.consume();
    }
    break;
  case KeyEvent::VK_Y:
    if (ctrl) {
      redo();
      e.consume();
    }
    break;
  default:
    break;
  }
}

void TextField::on_key_typed(KeyEvent &e) {
  auto code = e.get_key_char().get_code();

  // Some terminals deliver a Ctrl+letter chord as the control character
  // itself (Ctrl+A = 0x01), with no modifier bit set: run the same command.
  if (code > 0 and code < 0x20 and not (e.modifiers & InputEvent::CTRL_DOWN)) {
    switch (code) {
    case 0x01:
      select_all();
      e.consume();
      return;
    case 0x03:
      copy();
      e.consume();
      return;
    case 0x16:
      paste();
      e.consume();
      return;
    case 0x18:
      cut();
      e.consume();
      return;
    case 0x1a:
      undo();
      e.consume();
      return;
    case 0x19:
      redo();
      e.consume();
      return;
    default:
      return;
    }
  }

  if (not this->editable or not is_printable(e)) {
    return;
  }
  insert_typed(code);
  e.consume();
}

}
