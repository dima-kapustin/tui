#pragma once

// TextArea - a lightweight, viewport-friendly text editor component for very
// large files (see TextBuffer). Swing's JTextArea analog: it implements the
// Scrollable contract so a ScrollPane scrolls it by lines, and paints only
// the visible rows from the buffer.
//
// Features:
//   * caret movement / typing / backspace / delete / enter (byte-accurate)
//   * selection with Shift+arrows / Shift+click; Cut (Ctrl+X), Copy
//     (Ctrl+Insert, and Ctrl+C where the terminal delivers it), Paste (Ctrl+V)
//     and Select All (Ctrl+A) through an in-process Clipboard; Ctrl+Z / Ctrl+Y
//     undo and redo typing and backspace runs as single steps
//   * optional visible whitespace (space -> middle dot, tab -> arrow)
//   * incremental string and regexp search (F3), F4 toggles regexp,
//     F5 toggles whitespace
//   * keyboard input is received while the area is the focus owner; keys are
//     delivered through a listener on the owning window, because the
//     framework dispatches keys to windows, not to the focus owner.

#include <tui++/Component.h>
#include <tui++/Scrollable.h>
#include <tui++/TextBuffer.h>
#include <tui++/Timer.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace tui {

// The area receives keys through this forwarder (see TextArea.cpp): it is
// registered on the area's owning window and forwards to the area while the
// area is the keyboard focus owner. Declared here so TextArea can grant it
// access to the key handlers.
class TextAreaKeyForwarder;

class TextArea: public Component, public Scrollable {
  using base = Component;

public:
  TextArea();
  ~TextArea() override;

  void set_buffer(std::shared_ptr<TextBuffer> const &buffer) {
    this->buffer = buffer;
    this->caret = 0;
    this->sel_anchor = 0;
    this->undo_stack.clear();
    this->redo_stack.clear();
    invalidate_match();
    refresh_caret_geometry();
    refresh_view_size();
    repaint();
  }

  std::shared_ptr<TextBuffer> get_buffer() const {
    return this->buffer;
  }

  // The view's content size in cells (used by the scroll pane layout). The
  // height is the number of lines scanned so far and grows as the user scrolls
  // through the file.
  void refresh_view_size();

  void set_readonly(bool value) {
    this->readonly = value;
  }

  bool is_readonly() const {
    return this->readonly;
  }

  void set_show_whitespace(bool value) {
    if (this->show_whitespace != value) {
      this->show_whitespace = value;
      repaint();
    }
  }

  bool is_show_whitespace() const {
    return this->show_whitespace;
  }

  // Caret as a byte offset into the buffer.
  void set_caret(std::uint64_t offset);
  std::uint64_t get_caret() const {
    return this->caret;
  }

  // Text cursor ------------------------------------------------------------

  // The shape of the text cursor.
  enum class CaretForm {
    BLOCK,     // the whole cell inverted (the default)
    UNDERLINE  // a thin cursor: the glyph is drawn underlined
  };

  void set_caret_form(CaretForm form) {
    if (this->caret_form != form) {
      this->caret_form = form;
      repaint_caret_cell();
    }
  }

  CaretForm get_caret_form() const {
    return this->caret_form;
  }

  // The blink period of the cursor: the caret alternates between shown and
  // hidden every `rate`. Zero makes the cursor steady (always shown while the
  // area has the focus). Like Swing's DefaultCaret, a caret move or an edit
  // shows the caret solid again and restarts the blink.
  void set_caret_blink_rate(std::chrono::milliseconds rate) {
    if (this->caret_blink_rate != rate) {
      this->caret_blink_rate = rate;
      if (rate > std::chrono::milliseconds::zero()) {
        this->caret_on = true;
        this->blink_timer.set_period(rate);
        this->blink_timer.start();
      } else {
        this->blink_timer.stop();
        this->caret_on = true;
      }
      repaint_caret_cell();
    }
  }

  std::chrono::milliseconds get_caret_blink_rate() const {
    return this->caret_blink_rate;
  }

  // Whether the cursor is drawn at all (while the area has the focus). Off
  // hides it entirely; back on shows it solid and restarts the blink.
  void set_caret_visible(bool visible) {
    if (this->caret_visible != visible) {
      this->caret_visible = visible;
      if (visible) {
        restart_caret_blink();
      } else {
        this->blink_timer.stop();
        repaint_caret_cell();
      }
    }
  }

  bool is_caret_visible() const {
    return this->caret_visible;
  }

  // Selection / clipboard editing (the Edit menu operations) -----------------

  // True when a (non-empty) range is selected between the anchor and caret.
  bool has_selection() const {
    return this->sel_anchor != this->caret;
  }

  // The selected byte range, (start, end); start == end when none.
  std::pair<std::uint64_t, std::uint64_t> get_selection() const {
    auto [start, end] = selection_bounds();
    return { start, end };
  }

  // Copies the selection into the Clipboard. Safe without a selection.
  void copy();

  // Copies the selection and deletes it (a no-op in read-only mode).
  void cut();

  // Inserts the Clipboard content at the caret (replacing the selection).
  void paste();

  // Selects the whole content.
  void select_all();

  // Undo/redo of edits (typing runs and backspaces coalesce into one step).
  void undo();
  void redo();
  bool can_undo() const {
    return not this->undo_stack.empty();
  }
  bool can_redo() const {
    return not this->redo_stack.empty();
  }

  // Deletes the selection when there is one, otherwise the character before
  // (backward) / after (forward) the caret.
  void delete_backward();
  void delete_forward();

  // Scrolls so the caret is visible; no-op when the area is not inside a
  // viewport.
  void scroll_caret_to_visible();

  // Returns the nearest ancestor Viewport, if any.
  std::shared_ptr<class Viewport> get_viewport() const;

  // Asks the keyboard focus manager to make this area the key consumer.
  void request_input_focus();

  // Visible portion of the content (line of the top row).
  int get_top_line() const;

  // Search -------------------------------------------------------------

  // Runs a plain string (regexp=false) or regexp search for `pattern`,
  // starting at the caret when `from_caret`, wrapping at the end.
  // Returns true when a match was found and the caret moved to it.
  bool find_next(std::string const &pattern, bool regexp, bool forward = true);

  // Whether the next search runs as an ECMAScript regexp (F4 toggles).
  bool is_search_regexp() const {
    return this->search_regexp;
  }

  // Whether the search entry line is being edited at the bottom row.
  bool is_search_mode() const {
    return this->search_mode;
  }

  // The last match, as a byte range (end = start when none).
  std::pair<std::uint64_t, std::uint64_t> get_last_match() const {
    return { this->match_start, this->match_end };
  }

  // A transient status/message line (drawn at the bottom row).
  void show_message(std::string const &message);

  // Scrollable -----------------------------------------------------------

  Dimension get_preferred_scrollable_viewport_size() const override {
    return { 80, 24 };
  }

  int get_scrollable_unit_increment(Rectangle const &, Orientation) const override {
    return 1;
  }

  int get_scrollable_block_increment(Rectangle const &visible_rect, Orientation orientation) const override {
    return orientation == Orientation::VERTICAL ? std::max(1, visible_rect.height - 1) : std::max(1, visible_rect.width - 1);
  }

  bool get_scrollable_tracks_viewport_width() const override {
    return true;
  }

  bool get_scrollable_tracks_viewport_height() const override {
    return false;
  }

protected:
  virtual void paint(Graphics &g) override;
  virtual void add_notify() override;

  // Called by make_component once the area is owned by a shared pointer:
  // mouse listeners are registered here (add_listener needs shared ownership
  // to reach the containing window).
  void init() override;

private:
  friend class TextAreaKeyForwarder;

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  void on_key_pressed(KeyEvent &e);
  void on_key_typed(KeyEvent &e);

  void move_caret_left(std::uint64_t &offset) const;
  void move_caret_right(std::uint64_t &offset) const;
  void move_caret_line(int delta, int desired_cell, bool extend);
  void page(int delta, bool extend);

  // Byte offset of the char at `cell` cells from the start of line `line`
  // (clamped to the line end).
  std::uint64_t cell_to_offset(std::uint64_t line, int cell) const;

  // Recomputed caret line/cell caches; call after every caret move.
  void refresh_caret_geometry();

  void insert_text(std::string const &text);

  void invalidate_match() {
    this->match_start = this->match_end = UINT64_MAX;
  }

  bool is_match(std::uint64_t offset) const {
    return this->match_start != UINT64_MAX and offset >= this->match_start and offset < this->match_end;
  }

  // True when `offset` falls inside the selected range.
  bool is_selected(std::uint64_t offset) const {
    auto [start, end] = selection_bounds();
    return offset >= start and offset < end;
  }

  // (start, end) of the selection; equal when none.
  std::pair<std::uint64_t, std::uint64_t> selection_bounds() const {
    return { std::min(this->sel_anchor, this->caret), std::max(this->sel_anchor, this->caret) };
  }

  // Moves the caret to `offset`; with `extend` the selection anchor stays put
  // (shift-arrow selection), otherwise the selection collapses.
  void place_caret(std::uint64_t offset, bool extend);

  // Damages only the caret's cell, wherever it is (clipped when off-screen).
  void repaint_caret_cell();

  // Damages the visible part of file rows [first, last] (both inclusive), the
  // area of a caret move or a selection change.
  void repaint_caret_rows(std::uint64_t first, std::uint64_t last);

  // Damages the visible rows from `first_file_line` down to the bottom of the
  // viewport: the area an edit can change (the edited row plus every row an
  // inserted/deleted line would shift).
  void repaint_from_line(std::uint64_t first_file_line);

  // The file row of the viewport's last visible row (used to bound the
  // damaged regions above).
  std::uint64_t last_visible_line() const;

  // The number of the view's rows a viewport can show at once (all of them
  // when the view is not in one). The message/search row, when shown, is the
  // last of these rows.
  int visible_row_count() const;

  // Repaints only the message/search row (the bottom visible row): the only
  // row an entry in the search box or a status message changes.
  void repaint_message_row();

  // Repaints the rows of the current search-match highlight, if any. Call it
  // right before the match is cleared or replaced, so no stale highlight
  // survives the change.
  void repaint_match_rows();

  // True when the caret is drawn in the current frame: enabled, in the "on"
  // phase of the blink, and the area is the keyboard focus owner (Swing
  // paints its caret only then).
  bool is_caret_showing() const;

  // The blink tick: toggles the caret's phase and redraws its cell.
  void blink_tick();

  // Called after every caret move and edit: the caret is shown solid again
  // and the blink restarts from its "on" phase (Swing's DefaultCaret does
  // the same, so typing never hides the caret mid-word).
  void restart_caret_blink();

  // The area follows the keyboard focus: the caret is redrawn (and the blink
  // restarted) when the focus owner changes. `focus_gained` tells the new
  // state: the FOCUS_LOST event is dispatched before the keyboard focus
  // manager switches its owner, so is_focus_owner() cannot answer then.
  void focus_changed(bool focus_gained);

  // One undoable edit: [offset, offset + delete_len) replaced by `replacement`.
  void apply_edit(std::uint64_t offset, std::uint64_t delete_len, std::string_view replacement);

  // Pushes an edit onto the undo stack (merging contiguous typing/backspace
  // runs, dropping the redo stack).
  void record_edit(std::uint64_t offset, std::string removed, std::string inserted);

  void notify_window();

  std::shared_ptr<TextBuffer> buffer = TextBuffer::create_empty();
  std::uint64_t caret = 0;
  std::uint64_t caret_line = 0;
  int caret_cell = 0;
  bool readonly = false;
  bool show_whitespace = false;

  // Text cursor configuration and state. `caret_on` is the blink phase; it is
  // forced on (and the blink restarted) by every caret move and edit.
  CaretForm caret_form = CaretForm::BLOCK;
  std::chrono::milliseconds caret_blink_rate { 530 };
  bool caret_visible = true;
  bool caret_on = true;
  Timer blink_timer { std::chrono::milliseconds(530), [this] {
    blink_tick();
  } };

  // One edge of the selection (the other is the caret). Equal when collapsed.
  std::uint64_t sel_anchor = 0;

  struct Edit {
    std::uint64_t offset;
    std::string removed;
    std::string inserted;
  };
  std::vector<Edit> undo_stack;
  std::vector<Edit> redo_stack;

  std::uint64_t match_start = UINT64_MAX;
  std::uint64_t match_end = UINT64_MAX;
  std::string message;
  std::string search_pattern;
  bool search_mode = false;
  bool search_regexp = false;

  int64_t last_notified_lines = -1;
};

}
