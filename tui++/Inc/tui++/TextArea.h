#pragma once

// TextArea - a lightweight, viewport-friendly text editor component for very
// large files (see TextBuffer). Swing's JTextArea analog: it implements the
// Scrollable contract so a ScrollPane scrolls it by lines, and paints only
// the visible rows from the buffer.
//
// Features:
//   * caret movement / typing / backspace / delete / enter (byte-accurate);
//     Ctrl+Left/Right jump to the start/end of the current line, Ctrl+Up/
//     Down page up/down, Home/End move to line start/end, Ctrl+Home/End to
//     the document edges
//   * text selection: Shift+arrows / Shift+click, and drags with the mouse;
//     column (rectangular) selection with Alt+drag and Alt+Shift+arrows, or
//     in the column-select mode (F8 in the demo) with every selection gesture
//   * Cut (Ctrl+X), Copy (Ctrl+Insert, and Ctrl+C where the terminal delivers
//     it), Paste (Ctrl+V) and Select All (Ctrl+A) through an in-process
//     Clipboard; a column selection copies each row's covered cells joined
//     with newlines, and typing/backspace/delete/paste replace it
//   * Ctrl+Z / Ctrl+Y undo and redo typing and backspace runs as single
//     steps; a column-block edit undoes/redoes as one step as well
//   * optional visible whitespace (space -> middle dot, tab -> arrow)
//   * incremental string and regexp search (F3), F4 toggles regexp,
//     F5 toggles whitespace
//   * an optional occurrence highlight (set_occurrence_highlight): every
//     occurrence of the word at the caret -- or of the selection -- that is
//     visible on screen is highlighted; only the visible rows are scanned, so
//     the cost is bounded by the screen, not by the file
//   * keyboard input is received while the area is the focus owner; keys are
//     delivered through a listener on the owning window, because the
//     framework dispatches keys to windows, not to the focus owner.

#include <tui++/Component.h>
#include <tui++/Scrollable.h>
#include <tui++/TextBuffer.h>
#include <tui++/TextComponent.h>
#include <tui++/Timer.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tui {

// The area receives keys through this forwarder (see TextArea.cpp): it is
// registered on the area's owning window and forwards to the area while the
// area is the keyboard focus owner. Declared here so TextArea can grant it
// access to the key handlers.
class TextAreaKeyForwarder;

// Receives the drags that follow a press on the area (see TextArea.cpp): a
// Component does not process MOUSE_DRAG events itself, the window dispatcher
// retargets them to the press target for screen listeners only.
class TextAreaDragObserver;

class TextArea: public Component, public Scrollable, public TextComponent {
  using base = Component;

public:
  TextArea();
  ~TextArea() override;

  void set_buffer(std::shared_ptr<TextBuffer> const &buffer) {
    this->buffer = buffer;
    this->caret = 0;
    this->sel_anchor = 0;
    this->sel_block = false;
    this->mouse_dragging = false;
    this->undo_stack.clear();
    this->redo_stack.clear();
    this->max_line_width = 0;
    this->max_line_width_stale = true;
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

  // Whether the area accepts edits (Swing's JTextComponent.isEditable, the
  // positive of setReadonly). A read-only area still selects and copies.
  bool is_editable() const override {
    return not this->readonly;
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

  // Whether the text wraps (Swing's JTextArea.setLineWrap). When off (the
  // Swing default) a long line keeps its natural width and the enclosing
  // ScrollPane shows a horizontal scroll bar; when on, the content width is
  // forced to the viewport width (long lines are cut at the right edge).
  void set_line_wrap(bool value) {
    if (this->line_wrap != value) {
      this->line_wrap = value;
      this->max_line_width_stale = true;
      refresh_view_size();
      repaint();
    }
  }

  bool is_line_wrap() const {
    return this->line_wrap;
  }

  // Occurrence highlight (an optional view feature) -------------------------

  // Whether the area highlights, as a pure view effect, every occurrence of
  // the word under the caret -- or of the selection, when there is one --
  // that is visible on screen. Swing has no such feature; the "occurrence
  // highlight" of VS Code and other editors is the model. The caret does not
  // move and the search state (F3's match) is not touched.
  //
  // The cost of the feature is bounded by the screen, never by the file: only
  // the rows the viewport shows are scanned, and only the bytes the paint
  // pass reads anyway, so a 10 GiB file highlights exactly as fast as a 10
  // byte one. A change of the matched text (the caret entered another word,
  // the selection changed, an edit rewrote the caret's word) damages the
  // visible rows, because every one of them may gain or lose a highlight.
  void set_occurrence_highlight(bool value);

  bool is_occurrence_highlight() const {
    return this->occurrence_highlight;
  }

  // The longest text the highlight matches: a longer one -- a selection
  // spanning a paragraph, Ctrl+A on a huge file -- is not a pattern worth
  // chasing through every visible row, so the highlight stays off.
  static constexpr std::size_t MAX_OCCURRENCE_TEXT = 128;

  // The text the highlight currently matches: the selection's bytes, or the
  // word around the caret when there is no selection (a column selection has
  // no single byte text and falls back to the caret's word too). Empty when
  // the feature is off, the caret is not on a word, or the text is longer
  // than MAX_OCCURRENCE_TEXT. A status line can show it.
  std::string get_occurrence_text() const;

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
  bool has_selection() const override {
    return this->sel_anchor != this->caret;
  }

  // The selected byte range, (start, end); start == end when none. A column
  // selection spans from its top-left corner to the caret's bottom-right
  // corner, so its byte range covers every row of the block.
  std::pair<std::uint64_t, std::uint64_t> get_selection() const {
    auto [start, end] = selection_bounds();
    return { start, end };
  }

  // True when the current selection is a column (rectangular) block instead
  // of a contiguous byte range.
  bool is_block_selection() const {
    return this->sel_block and this->sel_anchor != this->caret;
  }

  // Column-select mode: while on, every selection gesture -- Shift+arrows,
  // Shift+click and mouse drags -- selects a column block rather than a byte
  // range. The mode exists because Alt+Shift chords are unreliable on some
  // terminals and hosts (Windows switches input languages on Alt+Shift); an
  // explicit column gesture (Alt+drag, Alt+Shift+arrows) selects a block
  // whether the mode is on or off. Swing has no analog; VS Code's column
  // selection mode is the model.
  void set_column_select_mode(bool value) {
    this->column_select_mode = value;
  }

  bool is_column_select_mode() const {
    return this->column_select_mode;
  }

  // The corner rows and cells of a column (block) selection. The block spans
  // the whole rows [top, bottom] and, on each of them, the terminal cells
  // [left, right). Cells beyond a row's text end select nothing there (a
  // column selection never invents virtual space).
  struct BlockRect {
    std::uint64_t top;
    std::uint64_t bottom;
    int left;
    int right;
  };

  // The selection's block rectangle, when it is a column selection.
  std::optional<BlockRect> get_block() const {
    return block_rect();
  }

  // Copies the selection into the Clipboard. Safe without a selection.
  void copy() override;

  // Copies the selection and deletes it (a no-op in read-only mode).
  void cut() override;

  // Inserts the Clipboard content at the caret (replacing the selection).
  void paste() override;

  // Selects the whole content.
  void select_all() override;

  // Undo/redo of edits (typing runs and backspaces coalesce into one step).
  void undo() override;
  void redo() override;
  bool can_undo() const override {
    return not this->undo_stack.empty();
  }
  bool can_redo() const override {
    return not this->redo_stack.empty();
  }

  // Deletes the selection when there is one, otherwise the character before
  // (backward) / after (forward) the caret.
  void delete_backward();
  void delete_forward() override;

  // The keyboard's popup trigger opens the standard text menu at the caret.
  virtual Point get_popup_menu_location() const override;

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
  // The search honors get_search_options().
  bool find_next(std::string const &pattern, bool regexp, bool forward = true);

  // The options of the searches the area runs (find_next): whether case
  // differences are ignored and whether a match must be a whole word (see
  // SearchOptions). The regexp mode is per search (is_search_regexp).
  void set_search_options(SearchOptions const &options) {
    this->search_options = options;
  }

  SearchOptions const& get_search_options() const {
    return this->search_options;
  }

  // Whether the next search runs as an ECMAScript regexp (F4 toggles).
  bool is_search_regexp() const {
    return this->search_regexp;
  }

  // Whether the search entry line is being edited at the bottom row.
  bool is_search_mode() const {
    return this->search_mode;
  }

  // The pattern of the current (or last) search entry -- plain text or a
  // regexp, as reported by is_search_regexp(). It survives the entry closing,
  // so a "find all" action can repeat the last search.
  std::string const& get_search_pattern() const {
    return this->search_pattern;
  }

  // Closes the search entry (the pattern above stays, so a later "find all"
  // can repeat it). A no-op when the entry is not open.
  void close_search_entry() {
    if (this->search_mode) {
      this->search_mode = false;
      repaint_message_row();
      repaint_caret_cell();
    }
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
    return this->line_wrap;
  }

  bool get_scrollable_tracks_viewport_height() const override {
    return false;
  }

  // Called by the enclosing scroll pane before it applies a wheel scroll
  // (see ScrollPane::process_wheel): the rows the scroll is about to reveal
  // may not be indexed yet, so scan them and refresh the content height
  // before the viewport clamps the new position. Without this the wheel
  // would stop at the lazy scan frontier of a large file.
  void scrollable_prepare_wheel_scroll(Rectangle const &visible_rect, Orientation orientation, int amount) override {
    if (orientation != Orientation::VERTICAL or amount <= 0) {
      return;
    }
    auto target = visible_rect.y + amount;
    this->buffer->ensure_line(std::uint64_t(std::max(0, target)) + std::uint64_t(std::max(0, visible_rect.height)) + 2);
    refresh_view_size();
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
  friend class TextAreaDragObserver;

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  void on_key_pressed(KeyEvent &e);
  void on_key_typed(KeyEvent &e);

  // --- click gestures -------------------------------------------------------

  // The classic click gestures of a text component (see TextField): a
  // double-click selects the word under the pointer -- or the run of
  // separators, when the click lands on one -- and a triple-click the whole
  // line. The press that precedes the click already placed the caret and left
  // a fresh selection anchor; the click lays the selection over it. A word
  // gesture always selects a byte range, even in the column-select mode: a
  // column is the shape of drags and Shift gestures, not of a word click.
  void on_mouse_click(MouseClickEvent &e);

  // The byte range of the word (or of the run of separators the clicked
  // character belongs to) around cell `cell` of `line`. A click past the
  // line's text lands on its last character, and an empty line yields an
  // empty range at its start. The scan stops MAX_WORD_SCAN bytes away from
  // the clicked character, so a line of gigabytes cannot make a double-click
  // walk the file.
  std::pair<std::uint64_t, std::uint64_t> word_bounds(std::uint64_t line, int cell) const;

  void move_caret_left(std::uint64_t &offset) const;
  void move_caret_right(std::uint64_t &offset) const;

  // Moves the caret `delta` rows towards `desired_cell` (or a page); `extend`
  // keeps the selection anchor put (Shift), `block` gives a fresh or
  // converted selection the column shape (Alt+Shift, or Alt+drag).
  void move_caret_line(int delta, int desired_cell, bool extend, bool block = false);
  void page(int delta, bool extend, bool block = false);

  // Byte offset of the char at `cell` cells from the start of line `line`
  // (clamped to the line end).
  std::uint64_t cell_to_offset(std::uint64_t line, int cell) const;

  // (line, terminal-cell column) of byte offset `offset` (clamped to the
  // content end). Like refresh_caret_geometry it derives the line start from
  // the byte column the line index reports, so it stays right when an edit
  // just rewound the index.
  std::pair<std::uint64_t, int> offset_cell(std::uint64_t offset) const;

  // Recomputed caret line/cell caches; call after every caret move.
  void refresh_caret_geometry();

  void insert_text(std::string const &text);

  // --- content width (horizontal scrolling) ---------------------------------

  // The content width in cells: the viewport width when the text wraps, or
  // the longest line's width when it does not. Recomputes (and caches) the
  // longest-line width on its first call after the cache goes stale.
  int content_width();

  // The cell width of one line range (decoded like paint does, without the
  // viewport clip), used to size the horizontal scroll bar.
  std::uint64_t measure_line_cells(TextBuffer::LineRange const &range) const;

  // The widest line across the whole content, in cells.
  std::uint64_t measure_max_line_width() const;

  // Grows the cached content width if `line` is now wider than every line seen
  // so far (an edit made it longer). It never shrinks, so deleting the widest
  // line keeps a harmless extra horizontal range until the buffer is reset.
  void grow_max_line_width(std::uint64_t line);

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
  // (shift-arrow selection), otherwise the selection collapses. `block` gives
  // a selection that starts or extends here the column shape: a fresh
  // selection takes it, an existing range is converted to a block (an
  // Alt+Shift gesture from the middle of a selection). The column-select
  // mode has the same effect on every extend gesture.
  void place_caret(std::uint64_t offset, bool extend, bool block = false);

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

  // One undoable chunk edit: [offset, offset + removed.size()) was replaced
  // by `inserted` (removed or inserted is empty for a pure delete/insert).
  struct Edit {
    std::uint64_t offset;
    std::string removed;
    std::string inserted;
  };

  // One undo step: the chunk edits that belong together (a typing run is a
  // single chunk; a column-block operation records one chunk per edited row).
  // A step's chunks replay in descending-offset order -- the order in which
  // every recorded offset stays valid while the buffer's length changes --
  // with distinct offsets (a block's chunks never collide; the top row's
  // delete and the replacement text insert merge into one chunk).
  struct EditStep {
    std::vector<Edit> chunks;
  };
  std::vector<EditStep> undo_stack;
  std::vector<EditStep> redo_stack;

  // --- column (block) selection ------------------------------------------

  // The rectangle between the anchor and the caret corners, when the
  // selection is a column block. The anchor corner is measured on demand
  // (the caret corner is the caret's own geometry); rows [top, bottom],
  // cells [left, right) on each of them.
  std::optional<BlockRect> block_rect() const;

  // The bytes of `line` covered by the block cells [cell0, cell1), clamped to
  // the line's text end; start == end when the line has no text in the band.
  // `line` must already be indexed (read_line_ranges needs it known).
  std::pair<std::uint64_t, std::uint64_t> block_line_span(std::uint64_t line, int cell0, int cell1) const;

  // Deletes the column block (per-row deletions, one undo step) and leaves
  // the caret at the block's top-left corner.
  void delete_block();

  // Replaces the column block with `text`: the block's rows are deleted and
  // the text is inserted at the block's top-left corner, all as one undo
  // step (the edit ops call it when typing/pasting over a column selection).
  void replace_block(std::string const &text);

  // One undoable edit: [offset, offset + delete_len) replaced by `replacement`.
  void apply_edit(std::uint64_t offset, std::uint64_t delete_len, std::string_view replacement);

  // Pushes an edit onto the undo stack (merging contiguous typing/backspace
  // runs, dropping the redo stack).
  void record_edit(std::uint64_t offset, std::string removed, std::string inserted);

  // Pushes a whole undo step at once: the chunk edits of a column-block
  // operation (merged typing runs must never merge into it). Sorts the
  // chunks into replay order and drops the redo stack.
  void record_block_edit(std::vector<Edit> chunks);

  void notify_window();

  // --- occurrence highlight -------------------------------------------------

  // The word around the caret: the maximal run of word characters (the rule
  // TextField's word commands use), read from a bounded window around the
  // caret. Empty when the caret is not on or next to a word, or when the run
  // is longer than MAX_OCCURRENCE_TEXT bytes -- the cap that keeps the matched
  // text small enough for the per-row scan.
  std::string word_at_caret() const;

  // Called after a caret, selection or content change: when the highlight is
  // on and the text it would match now differs from `old_text`, the whole
  // visible band is damaged -- every row may have gained or lost highlights,
  // and the rows the change itself damaged do not cover them.
  void repaint_if_occurrence_changed(std::string const &old_text);

  // Damages the rows the viewport shows: the band the occurrence highlight
  // can ever change.
  void repaint_visible_rows();

  bool occurrence_highlight = false;

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

  // True while the selection has the column shape (see the public selection
  // section). A collapsed selection carries no shape: extending it with a
  // plain gesture makes a byte range, extending it with a column gesture
  // makes a block.
  bool sel_block = false;
  bool column_select_mode = false;

  // --- mouse drag selection ------------------------------------------------

  // A drag extends the selection from the press point (the anchor the press
  // left behind) to wherever the pointer went. The target row is clamped to
  // the content: a drag past the scanned frontier extends the lazy line
  // index, and one past the content end selects to the last line. The caret
  // is scrolled into view after every drag step, so dragging past the
  // viewport edge scrolls the pane behind the pointer.
  void on_mouse_drag(int x, int y);

  // Ends the drag: the button went up (the observer unregisters itself).
  void on_mouse_release();

  // The screen-level drag observer (see TextArea.cpp), registered while the
  // left button is down on this area; drags and the release that ends them
  // are retargeted to the area that received the press.
  void register_drag_observer();
  void unregister_drag_observer();

  // The left button is down on this area (a press was seen) and a drag is in
  // progress. `mouse_drag_block` is the press's column gesture: drags extend
  // the selection from wherever the press left it, in that shape.
  bool mouse_dragging = false;
  bool mouse_drag_block = false;
  std::shared_ptr<TextAreaDragObserver> drag_observer;

  std::uint64_t match_start = UINT64_MAX;
  std::uint64_t match_end = UINT64_MAX;
  std::string message;
  std::string search_pattern;
  SearchOptions search_options;
  bool search_mode = false;
  bool search_regexp = false;

  bool line_wrap = false; // Swing's JTextArea default: no line wrap, natural width

  // The content width in cells when line_wrap is off: the longest line seen so
  // far. Recomputed lazily (once, when it goes stale) and grown by edits; it
  // is never shrunk, so deleting the longest line leaves a harmless extra
  // horizontal range until the buffer is reset.
  std::uint64_t max_line_width = 0;
  bool max_line_width_stale = true;

  int64_t last_notified_lines = -1;
};

}
