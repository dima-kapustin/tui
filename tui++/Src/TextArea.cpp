#include <tui++/TextArea.h>
#include <tui++/Clipboard.h>
#include <tui++/Viewport.h>
#include <tui++/ScrollPane.h>
#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/Attributes.h>
#include <tui++/Screen.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/util/utf-8.h>
#include <tui++/util/unicode.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>

#include <algorithm>
#include <cstdint>

namespace tui {

namespace {

std::string to_utf8(char32_t code) {
  std::string out;
  if (code < 0x80) {
    out += char(code);
  } else if (code < 0x800) {
    out += char(0xC0 | (code >> 6));
    out += char(0x80 | (code & 0x3F));
  } else if (code < 0x10000) {
    out += char(0xE0 | (code >> 12));
    out += char(0x80 | ((code >> 6) & 0x3F));
    out += char(0x80 | (code & 0x3F));
  } else {
    out += char(0xF0 | (code >> 18));
    out += char(0x80 | ((code >> 12) & 0x3F));
    out += char(0x80 | ((code >> 6) & 0x3F));
    out += char(0x80 | (code & 0x3F));
  }
  return out;
}

int utf8_len(char const *p, std::size_t available) {
  auto first = std::uint8_t(*p);
  if (first < 0x80) {
    return 1;
  }
  auto n = first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
  return int(std::min<std::size_t>(n, available));
}

// Decodes the character starting at `p` (valid for `available` bytes).
char32_t decode_char(char const *p, std::size_t available) {
  auto len = utf8_len(p, available);
  char32_t code = 0;
  util::mb_to_c32(p, len, &code);
  return code;
}

// The number of content bytes needed to be reasonably sure `cells` terminal
// cells are covered by one read: at most 4 UTF-8 bytes per cell, plus slack
// for combining marks (which consume bytes but no cells).
std::uint64_t bytes_for_cells(std::uint64_t cells) {
  return cells * 4 + 128;
}

}

// The framework dispatches key events to windows, not to the focus owner, so
// the area registers a forwarder on its owning window while it is displayable.
// Defined outside the anonymous namespace so the friend declaration in the
// header (tui::TextAreaKeyForwarder) names the same class.
class TextAreaKeyForwarder final: public EventListener<KeyEvent> {
  std::weak_ptr<TextArea> area;

public:
  explicit TextAreaKeyForwarder(std::weak_ptr<TextArea> const &area) :
      area(area) {
  }

  virtual void key_pressed(KeyEvent &e) override {
    if (not e.consumed) {
      if (auto area = this->area.lock(); area and KeyboardFocusManager::single->get_focus_owner() == area) {
        area->on_key_pressed(e);
      }
    }
  }

  virtual void key_typed(KeyEvent &e) override {
    if (not e.consumed) {
      if (auto area = this->area.lock(); area and KeyboardFocusManager::single->get_focus_owner() == area) {
        area->on_key_typed(e);
      }
    }
  }
};

TextArea::TextArea() {
  // The same near-black as the viewport's default: the area only covers part
  // of the viewport, and the cells to its right show the viewport's color. A
  // one-unit difference here made every width growth repaint the exposed
  // column cell-by-cell (the "snake" the caret test guards against).
  set_background_color(Color { 12, 12, 16 });
  set_foreground_color(Color { 190, 196, 205 });
  set_name("text area");
}

void TextArea::init() {
  base::init();

  // The event coordinates are translated into this component's own space,
  // which is the *file* space (the viewport places the view at
  // -view_position), so a click row is already a file line. A Shift+click
  // extends the selection to the clicked spot instead of moving the caret.
  add_listener([this](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    auto line = std::uint64_t(std::max(0, e.y));
    if (this->buffer->known_line_count() > line) {
      auto extend = bool(e.modifiers & InputEvent::SHIFT_DOWN);
      place_caret(cell_to_offset(line, std::max(0, e.x)), extend);
    }
    request_input_focus();
    e.consume();
  });

  // The wheel is handled by the enclosing ScrollPane (see
  // ScrollPane::process_wheel): the pane scrolls by this area's unit
  // increment wherever the pointer is over it, the view's background
  // included, and gives the area a chance to extend its lazy line index
  // first (scrollable_prepare_wheel_scroll below).

  // The caret follows the keyboard focus: it is only painted while the area
  // is the focus owner, and regaining the focus restarts its blink. The id
  // tells the direction (the FOCUS_LOST event is dispatched to the old owner
  // before the focus manager records the switch).
  add_listener([this](FocusEvent &e) {
    focus_changed(e.id == FocusEvent::FOCUS_GAINED);
  });
}

TextArea::~TextArea() = default;

std::shared_ptr<Viewport> TextArea::get_viewport() const {
  for (auto parent = get_parent(); parent; parent = parent->get_parent()) {
    if (auto viewport = std::dynamic_pointer_cast<Viewport>(parent)) {
      return viewport;
    }
  }
  return {};
}

void TextArea::request_input_focus() {
  request_focus(false, FocusEvent::Cause::ACTIVATION);
}

int TextArea::get_top_line() const {
  if (auto viewport = get_viewport()) {
    return viewport->get_view_position().y;
  }
  return 0;
}

void TextArea::add_notify() {
  if (auto window = get_containing_window()) {
    window->add_listener(std::make_shared<TextAreaKeyForwarder>(std::static_pointer_cast<TextArea>(shared_from_this())));
  }
  base::add_notify();
}

void TextArea::notify_window() {
  for (auto parent = get_parent(); parent; parent = parent->get_parent()) {
    if (auto pane = std::dynamic_pointer_cast<ScrollPane>(parent)) {
      pane->revalidate();
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// caret movement helpers

void TextArea::move_caret_left(std::uint64_t &offset) const {
  if (offset == 0) {
    return;
  }
  --offset;
  // Step over UTF-8 continuation bytes back to the sequence head.
  while (offset > 0) {
    auto byte = this->buffer->read(offset, 1)[0];
    if ((std::uint8_t(byte) & 0xC0) != 0x80) {
      break;
    }
    --offset;
  }
}

void TextArea::move_caret_right(std::uint64_t &offset) const {
  auto total = this->buffer->length();
  if (offset >= total) {
    return;
  }
  auto probe = this->buffer->read(offset, 4);
  offset += std::uint64_t(utf8_len(probe.data(), probe.size()));
}

void TextArea::refresh_caret_geometry() {
  this->caret = std::min(this->caret, this->buffer->length());
  auto [line, byte_col] = this->buffer->offset_to_line(this->caret);
  this->caret_line = line;

  // Terminal-cell column of the caret within its line. The line start is the
  // caret offset minus its byte column (offset_to_line reports the column),
  // not buffer->line_start(line): an edit just rewound the lazy line index
  // to the anchor before it, so line_start would clamp to the content end
  // and the caret would measure from there (landing on column 0).
  auto start = this->caret - std::min(byte_col, this->caret);
  auto text = this->buffer->read(start, byte_col);
  auto cell = 0;
  auto pos = std::size_t(0);
  while (pos < text.size()) {
    auto code = decode_char(text.data() + pos, text.size() - pos);
    auto width = util::unicode::glyph_width(code);
    if (width > 0) {
      cell += width;
    }
    pos += std::size_t(utf8_len(text.data() + pos, text.size() - pos));
  }
  this->caret_cell = cell;
}

// Byte offset of the character whose leading cell column is >= `cell`, or the
// end of the line when `cell` is past it.
std::uint64_t TextArea::cell_to_offset(std::uint64_t line, int cell) const {
  if (line >= this->buffer->known_line_count()) {
    return this->buffer->length();
  }
  auto start = this->buffer->line_start(line);
  auto end = this->buffer->length();
  if (line + 1 < this->buffer->known_line_count()) {
    // The line ends at its newline (the byte before the next line's start).
    end = this->buffer->line_start(line + 1) - 1;
  }
  auto current = 0;
  auto offset = start;
  while (offset < end and current < cell) {
    auto probe = this->buffer->read(offset, 4);
    auto code = decode_char(probe.data(), probe.size());
    if (code == '\n') {
      break;
    }
    auto width = util::unicode::glyph_width(code);
    if (width > 0) {
      if (current + width > cell) {
        break;
      }
      current += width;
    }
    offset += std::uint64_t(utf8_len(probe.data(), probe.size()));
  }
  return std::min(offset, end);
}

void TextArea::place_caret(std::uint64_t offset, bool extend) {
  auto old_caret_line = this->caret_line;
  auto first = old_caret_line;
  auto last = old_caret_line;
  if (not extend and this->sel_anchor != this->caret) {
    // Collapsing a selection: the whole band between the anchor and the
    // caret loses its highlight, so it must be repainted as well.
    auto anchor_line = this->buffer->offset_to_line(this->sel_anchor).first;
    first = std::min(first, anchor_line);
    last = std::max(last, anchor_line);
  }
  this->caret = std::min(offset, this->buffer->length());
  if (not extend) {
    // A plain caret move collapses the selection (the anchor follows).
    this->sel_anchor = this->caret;
  }
  refresh_caret_geometry();
  scroll_caret_to_visible();
  // Only the band between the old and the new caret row changes (the
  // selection grows or shrinks there, and the caret moves there); the rows
  // beyond it keep their state and must not be repainted.
  repaint_caret_rows(std::min(first, this->caret_line), std::max(last, this->caret_line));
  restart_caret_blink();
}

void TextArea::set_caret(std::uint64_t offset) {
  auto old_caret_line = this->caret_line;
  auto first = old_caret_line;
  auto last = old_caret_line;
  if (this->sel_anchor != this->caret) {
    // The jump collapses the selection; erase its whole band.
    auto anchor_line = this->buffer->offset_to_line(this->sel_anchor).first;
    first = std::min(first, anchor_line);
    last = std::max(last, anchor_line);
  }
  // A deep jump erases the search highlight wherever it was.
  repaint_match_rows();
  this->caret = std::min(offset, this->buffer->length());
  this->sel_anchor = this->caret;
  invalidate_match();
  // A deep jump (search hit, Ctrl+End) may land beyond the scanned region;
  // index it first so the caret geometry does not rescan the whole gap.
  this->buffer->ensure_scanned_to(this->caret);
  refresh_caret_geometry();
  refresh_view_size();
  scroll_caret_to_visible();
  repaint_caret_rows(std::min(first, this->caret_line), std::max(last, this->caret_line));
  restart_caret_blink();
}

void TextArea::scroll_caret_to_visible() {
  auto viewport = get_viewport();
  if (not viewport) {
    return;
  }
  auto line = int(std::min<std::uint64_t>(this->caret_line, 1ull << 30));
  viewport->scroll_rect_to_visible({ this->caret_cell, std::max(0, line), 1, 1 });
}

// ---------------------------------------------------------------------------
// repaint damage: rows of the view, not the whole view
//
// The view child of a viewport spans the whole content and sits at
// -view_position (see Viewport::place_view), so file line `line` lives at
// view row `line`: a repaint rectangle that starts at that row needs no
// mapping. Every repaint below is clipped to the viewport (and to the
// screen) by the repaint path itself, so rows outside the visible area cost
// nothing, and rows that are visible but unchanged emit nothing (the shadow
// comparison in TextScreen::emit_row). This is what keeps a keystroke a
// one-row repaint instead of a full-screen one.

int TextArea::visible_row_count() const {
  auto rows = get_height();
  if (auto viewport = get_viewport()) {
    rows = std::min(rows, std::max(0, viewport->get_height()));
  }
  return std::max(0, rows);
}

std::uint64_t TextArea::last_visible_line() const {
  auto known = this->buffer->known_line_count();
  if (known == 0) {
    return 0;
  }
  // The row below the last visible one, exclusive: everything up to the
  // viewport's bottom edge (the message/search row included; repainting it
  // is harmless, it re-emits nothing when it did not change).
  auto top = std::uint64_t(std::max(0, get_top_line()));
  auto bottom = top + std::uint64_t(std::max(1, visible_row_count()));
  return bottom <= known ? bottom - 1 : known - 1;
}

void TextArea::repaint_caret_cell() {
  if (this->caret_line >= std::uint64_t(get_height())) {
    return;
  }
  // Two cells wide: the caret can overwrite the leading cell of a wide
  // glyph (its continuation cell is redrawn along as a side effect of the
  // row's emission rules). Rows and columns outside the viewport are
  // clipped by the repaint path.
  repaint(std::max(0, this->caret_cell), int(this->caret_line), 2, 1);
}

void TextArea::repaint_caret_rows(std::uint64_t first, std::uint64_t last) {
  auto lo = std::min(first, last);
  auto hi = std::max(first, last);
  auto height = std::uint64_t(std::max(0, get_height()));
  if (lo >= height) {
    return; // nothing of the band is inside the view's content
  }
  hi = std::min(hi, height - 1);
  repaint(0, int(lo), get_width(), int(hi - lo + 1));
}

void TextArea::repaint_from_line(std::uint64_t first_file_line) {
  // An edit that inserts or removes a line break shifts every visible row
  // below it; damage the band from the edited row to the viewport's bottom.
  auto first = std::max(first_file_line, std::uint64_t(std::max(0, get_top_line())));
  auto last = last_visible_line();
  if (first > last) {
    return; // the edit is below the visible rows: nothing changed on screen
  }
  repaint(0, int(first), get_width(), int(last - first + 1));
}

void TextArea::repaint_message_row() {
  auto top = std::uint64_t(std::max(0, get_top_line()));
  auto row = int(top) + std::max(0, visible_row_count() - 1);
  if (row >= 0 and row < get_height()) {
    repaint(0, row, get_width(), 1);
  }
}

void TextArea::repaint_match_rows() {
  if (this->match_start == UINT64_MAX or this->match_end == UINT64_MAX or this->match_start >= this->match_end) {
    return;
  }
  auto known = this->buffer->known_line_count();
  if (known == 0) {
    return;
  }
  auto first = this->buffer->offset_to_line(this->match_start).first;
  auto last = this->buffer->offset_to_line(this->match_end - 1).first;
  repaint_caret_rows(first, last);
}

// ---------------------------------------------------------------------------
// the text cursor: form, blink and the keyboard focus

bool TextArea::is_caret_showing() const {
  // Swing paints its caret only while its text component is the focus owner;
  // the search entry replaces the document caret while it is being edited.
  return this->caret_visible and not this->search_mode and is_focus_owner() //
      and (this->caret_blink_rate <= std::chrono::milliseconds::zero() or this->caret_on);
}

void TextArea::blink_tick() {
  // The caret is only drawn (and therefore only worth repainting) while it
  // is enabled, the area is the focus owner, and the search entry is not up.
  if (this->caret_visible and not this->search_mode and is_focus_owner()) {
    this->caret_on = not this->caret_on;
    repaint_caret_cell();
  }
}

void TextArea::restart_caret_blink() {
  // Every caret move and edit shows the caret solid again and restarts the
  // blink from its "on" phase (Swing's DefaultCaret does the same, so
  // typing never hides the caret mid-word).
  this->caret_on = true;
  if (this->caret_blink_rate > std::chrono::milliseconds::zero() and is_focus_owner()) {
    this->blink_timer.start();
  }
  if (is_caret_showing()) {
    repaint_caret_cell();
  }
}

void TextArea::focus_changed(bool focus_gained) {
  if (focus_gained) {
    // The caret appears again; the blink restarts from its on phase.
    restart_caret_blink();
  } else {
    // The caret disappears with the focus (and the blink stops until the
    // focus comes back).
    this->blink_timer.stop();
    this->caret_on = true;
    if (this->caret_visible) {
      repaint_caret_cell();
    }
  }
}

// Number of '\n' in the buffer range [offset, offset + length). Read in
// bounded windows: the range is usually a selection being replaced, which
// can be arbitrarily large.
static std::uint64_t count_buffer_newlines(TextBuffer const &buffer, std::uint64_t offset, std::uint64_t length) {
  auto count = std::uint64_t { 0 };
  constexpr auto CHUNK = std::uint64_t(64) << 10;
  auto end = std::min(offset + length, buffer.length());
  for (auto pos = offset; pos < end;) {
    auto window = buffer.read(pos, std::min(CHUNK, end - pos));
    count += std::uint64_t(std::count(window.begin(), window.end(), '\n'));
    pos += window.size();
  }
  return count;
}

void TextArea::move_caret_line(int delta, int desired_cell, bool extend) {
  auto old_caret_line = this->caret_line;
  auto first = old_caret_line;
  auto last = old_caret_line;
  if (not extend and this->sel_anchor != this->caret) {
    auto anchor_line = this->buffer->offset_to_line(this->sel_anchor).first;
    first = std::min(first, anchor_line);
    last = std::max(last, anchor_line);
  }
  auto target = std::int64_t(this->caret_line) + delta;
  if (target < 0) {
    target = 0;
  }
  auto known = std::int64_t(this->buffer->ensure_line(std::uint64_t(target) + 1));
  target = std::min(target, known - 1);
  auto offset = cell_to_offset(std::uint64_t(target), desired_cell);
  this->caret = offset;
  if (not extend) {
    this->sel_anchor = this->caret;
  }
  refresh_caret_geometry();
  scroll_caret_to_visible();
  repaint_caret_rows(std::min(first, this->caret_line), std::max(last, this->caret_line));
  restart_caret_blink();
}

void TextArea::page(int delta, bool extend) {
  auto viewport = get_viewport();
  auto step = std::max(1, (viewport ? viewport->get_height() : get_height()) - 1);
  auto top = std::int64_t(get_top_line());
  auto old_caret_line = this->caret_line;
  auto first = old_caret_line;
  auto last = old_caret_line;
  if (not extend and this->sel_anchor != this->caret) {
    auto anchor_line = this->buffer->offset_to_line(this->sel_anchor).first;
    first = std::min(first, anchor_line);
    last = std::max(last, anchor_line);
  }

  auto target_top = top + std::int64_t(delta) * step;
  if (target_top < 0) {
    target_top = 0;
  }
  // The page may cover rows the lazy index has not seen yet; extend the index
  // (and with it the view content height) before the viewport clamps.
  this->buffer->ensure_line(std::uint64_t(target_top) + std::uint64_t((viewport ? viewport->get_height() : get_height())) + 2);
  refresh_view_size();
  if (viewport) {
    auto position = viewport->get_view_position();
    viewport->set_view_position(position.x, int(std::min<std::int64_t>(target_top, 1 << 30)));
  }
  top = std::int64_t(get_top_line());

  // Move the caret by the same page and keep it on a visible row.
  auto caret_target = std::int64_t(this->caret_line) + std::int64_t(delta) * step;
  caret_target = std::clamp<std::int64_t>(caret_target, top, top + step);
  auto known = std::int64_t(this->buffer->known_line_count());
  caret_target = std::min(caret_target, known - 1);
  auto desired = this->caret_cell;
  this->caret = cell_to_offset(std::uint64_t(std::max<std::int64_t>(0, caret_target)), desired);
  if (not extend) {
    this->sel_anchor = this->caret;
  }
  refresh_caret_geometry();
  // A page that actually scrolled repainted the whole viewport already
  // (Viewport::set_view_position); the row band below only adds the rows of
  // a page that stayed put (or was clamped at the content edge), plus the
  // erased selection band of a collapsing page move.
  repaint_caret_rows(std::min(first, this->caret_line), std::max(last, this->caret_line));
  restart_caret_blink();
}

// ---------------------------------------------------------------------------
// editing

void TextArea::record_edit(std::uint64_t offset, std::string removed, std::string inserted) {
  // Very large edits (a multi-megabyte paste) would pin that much memory in
  // the undo stack; keep them out of history instead of unbounded growth.
  constexpr std::size_t MAX_RECORDED_BYTES = 512 << 10;
  if (removed.size() + inserted.size() > MAX_RECORDED_BYTES) {
    this->undo_stack.clear();
    this->redo_stack.clear();
    return;
  }

  // Merge a typing run into the previous pure insert, and a backspace run
  // into the previous pure delete (contiguous, same direction).
  auto merged = false;
  if (not this->undo_stack.empty()) {
    auto &last = this->undo_stack.back();
    if (removed.empty() and last.removed.empty() and last.inserted.size() + inserted.size() <= MAX_RECORDED_BYTES) {
      if (last.offset + last.inserted.size() == offset) {
        last.inserted += inserted;
        merged = true;
      }
    } else if (inserted.empty() and last.inserted.empty() and last.removed.size() + removed.size() <= MAX_RECORDED_BYTES) {
      if (offset + removed.size() == last.offset) {
        last.removed = removed + last.removed;
        last.offset = offset;
        merged = true;
      }
    }
  }
  if (not merged) {
    this->undo_stack.emplace_back(offset, std::move(removed), std::move(inserted));
  }
  // A fresh edit invalidates the redo history (Swing's UndoManager does the
  // same when an edit is added after an undo).
  this->redo_stack.clear();

  constexpr std::size_t MAX_UNDO_STEPS = 200;
  if (this->undo_stack.size() > MAX_UNDO_STEPS) {
    this->undo_stack.erase(this->undo_stack.begin());
  }
}

void TextArea::apply_edit(std::uint64_t offset, std::uint64_t delete_len, std::string_view replacement) {
  auto removed = this->buffer->read(offset, delete_len);
  this->buffer->replace(offset, delete_len, replacement);
  record_edit(offset, std::move(removed), std::string(replacement));
}

void TextArea::insert_text(std::string const &text) {
  if (text.empty()) {
    return;
  }
  if (this->search_mode) {
    // The document caret is not drawn while the search entry is edited; only
    // the entry row changes.
    this->search_pattern += text;
    repaint_message_row();
    return;
  }
  if (this->readonly) {
    return;
  }

  // Typing replaces the selection; without one it inserts at the caret.
  auto [start, end] = selection_bounds();
  auto offset = has_selection() ? start : this->caret;
  auto delete_len = end - start;
  auto first_line = this->buffer->offset_to_line(offset).first;
  auto inserted_newlines = std::uint64_t(std::count(text.begin(), text.end(), '\n'));
  auto removed_newlines = delete_len > 0 ? count_buffer_newlines(*this->buffer, offset, delete_len) : 0;
  repaint_match_rows(); // the edit erases the current search highlight
  apply_edit(offset, delete_len, text);
  this->caret = offset + text.size();
  this->sel_anchor = this->caret;
  invalidate_match();
  refresh_caret_geometry();
  grow_max_line_width(this->caret_line);
  refresh_view_size();
  scroll_caret_to_visible();
  if (inserted_newlines == removed_newlines) {
    // The rows below the edit did not move; the changed rows are the edited
    // one (and the replacement when it spans rows without changing their
    // count), all covered by the caret's row span.
    repaint_caret_rows(first_line, this->caret_line);
  } else {
    // A line was inserted or removed: every visible row below the edit
    // shifted, so the damage runs to the viewport's bottom.
    repaint_from_line(first_line);
  }
  restart_caret_blink();
}

void TextArea::delete_backward() {
  if (this->readonly) {
    return;
  }
  auto [start, end] = selection_bounds();
  auto offset = start;
  auto delete_len = end - start;
  if (not has_selection()) {
    if (this->caret == 0) {
      return;
    }
    offset = this->caret;
    move_caret_left(offset);
    delete_len = this->caret - offset;
  }
  auto first_line = this->buffer->offset_to_line(offset).first;
  auto removed_newlines = count_buffer_newlines(*this->buffer, offset, delete_len);
  repaint_match_rows();
  apply_edit(offset, delete_len, "");
  this->caret = offset;
  this->sel_anchor = this->caret;
  invalidate_match();
  refresh_caret_geometry();
  grow_max_line_width(this->caret_line);
  refresh_view_size();
  if (removed_newlines > 0) {
    repaint_from_line(first_line);
  } else {
    repaint_caret_rows(first_line, this->caret_line);
  }
  restart_caret_blink();
}

void TextArea::delete_forward() {
  if (this->readonly) {
    return;
  }
  auto [start, end] = selection_bounds();
  auto offset = start;
  auto delete_len = end - start;
  if (not has_selection()) {
    if (this->caret >= this->buffer->length()) {
      return;
    }
    auto end_offset = this->caret;
    move_caret_right(end_offset);
    offset = this->caret;
    delete_len = end_offset - this->caret;
  }
  auto first_line = this->buffer->offset_to_line(offset).first;
  auto removed_newlines = count_buffer_newlines(*this->buffer, offset, delete_len);
  repaint_match_rows();
  apply_edit(offset, delete_len, "");
  this->caret = offset;
  this->sel_anchor = this->caret;
  invalidate_match();
  refresh_caret_geometry();
  grow_max_line_width(this->caret_line);
  refresh_view_size();
  if (removed_newlines > 0) {
    repaint_from_line(first_line);
  } else {
    repaint_caret_rows(first_line, this->caret_line);
  }
  restart_caret_blink();
}

// ---------------------------------------------------------------------------
// selection / clipboard / undo

void TextArea::copy() {
  if (not has_selection()) {
    return;
  }
  auto [start, end] = selection_bounds();
  Clipboard::set_text(this->buffer->read(start, end - start));
}

void TextArea::cut() {
  copy();
  delete_backward(); // deletes the selection when there is one
}

void TextArea::paste() {
  if (this->readonly) {
    return;
  }
  if (Clipboard::has_text()) {
    insert_text(Clipboard::get_text());
  }
}

void TextArea::select_all() {
  this->buffer->ensure_scanned_to(this->buffer->length());
  repaint_match_rows();
  this->sel_anchor = 0;
  this->caret = this->buffer->length();
  invalidate_match();
  refresh_caret_geometry();
  refresh_view_size();
  scroll_caret_to_visible();
  // The whole visible band turns selected.
  repaint_from_line(0);
  restart_caret_blink();
}

void TextArea::undo() {
  if (this->undo_stack.empty() or this->readonly) {
    return;
  }
  auto edit = this->undo_stack.back();
  this->undo_stack.pop_back();
  // Undo the edit: remove what it inserted and restore what it removed.
  auto first_line = this->buffer->offset_to_line(edit.offset).first;
  auto inserted_newlines = std::uint64_t(std::count(edit.removed.begin(), edit.removed.end(), '\n'));
  auto removed_newlines = std::uint64_t(std::count(edit.inserted.begin(), edit.inserted.end(), '\n'));
  repaint_match_rows();
  this->buffer->replace(edit.offset, edit.inserted.size(), edit.removed);
  this->caret = std::min(edit.offset + edit.removed.size(), this->buffer->length());
  this->sel_anchor = this->caret;
  this->redo_stack.emplace_back(std::move(edit));
  invalidate_match();
  refresh_caret_geometry();
  grow_max_line_width(this->caret_line);
  refresh_view_size();
  scroll_caret_to_visible();
  if (inserted_newlines == removed_newlines) {
    repaint_caret_rows(first_line, this->caret_line);
  } else {
    repaint_from_line(first_line);
  }
  restart_caret_blink();
}

void TextArea::redo() {
  if (this->redo_stack.empty() or this->readonly) {
    return;
  }
  auto edit = this->redo_stack.back();
  this->redo_stack.pop_back();
  // Redo the edit: remove what it removed and restore what it inserted.
  auto first_line = this->buffer->offset_to_line(edit.offset).first;
  auto inserted_newlines = std::uint64_t(std::count(edit.inserted.begin(), edit.inserted.end(), '\n'));
  auto removed_newlines = std::uint64_t(std::count(edit.removed.begin(), edit.removed.end(), '\n'));
  repaint_match_rows();
  this->buffer->replace(edit.offset, edit.removed.size(), edit.inserted);
  this->caret = std::min(edit.offset + edit.inserted.size(), this->buffer->length());
  this->sel_anchor = this->caret;
  this->undo_stack.emplace_back(std::move(edit));
  invalidate_match();
  refresh_caret_geometry();
  grow_max_line_width(this->caret_line);
  refresh_view_size();
  scroll_caret_to_visible();
  if (inserted_newlines == removed_newlines) {
    repaint_caret_rows(first_line, this->caret_line);
  } else {
    repaint_from_line(first_line);
  }
  restart_caret_blink();
}

// ---------------------------------------------------------------------------
// key handling

void TextArea::on_key_pressed(KeyEvent &e) {
  auto ctrl = bool(e.modifiers & InputEvent::CTRL_DOWN);
  auto shift = bool(e.modifiers & InputEvent::SHIFT_DOWN);
  switch (e.get_key_code()) {
  case KeyEvent::VK_LEFT:
    if (not this->search_mode and not ctrl) {
      auto offset = this->caret;
      move_caret_left(offset);
      place_caret(offset, shift); // Shift+Left extends the selection
    }
    e.consume();
    break;
  case KeyEvent::VK_RIGHT:
    if (not this->search_mode and not ctrl) {
      auto offset = this->caret;
      move_caret_right(offset);
      place_caret(offset, shift);
    }
    e.consume();
    break;
  case KeyEvent::VK_UP:
    if (not this->search_mode) {
      move_caret_line(-1, this->caret_cell, shift);
    }
    e.consume();
    break;
  case KeyEvent::VK_DOWN:
    if (not this->search_mode) {
      move_caret_line(1, this->caret_cell, shift);
    }
    e.consume();
    break;
  case KeyEvent::VK_PAGE_UP:
    if (not this->search_mode) {
      page(-1, shift);
    }
    e.consume();
    break;
  case KeyEvent::VK_PAGE_DOWN:
    if (not this->search_mode) {
      page(1, shift);
    }
    e.consume();
    break;
  case KeyEvent::VK_HOME:
    if (not this->search_mode) {
      if (ctrl) {
        place_caret(0, shift);
      } else {
        place_caret(this->buffer->line_start(this->caret_line), shift);
      }
    }
    e.consume();
    break;
  case KeyEvent::VK_END:
    if (not this->search_mode) {
      if (ctrl) {
        // Ctrl+End: the very end of the content (a full scan of the lazy
        // line index is the price of knowing where that is).
        this->buffer->scan_to_end();
        refresh_view_size();
        place_caret(this->buffer->length(), shift);
      } else {
        // End of the text of the caret's line: the byte before its newline,
        // or the content end when the line is the last one. An empty final
        // line (after a trailing newline) is "its own end": the content end.
        this->buffer->ensure_line(this->caret_line + 2);
        auto known = this->buffer->known_line_count();
        auto end = this->caret_line + 1 < known ? this->buffer->line_start(this->caret_line + 1) - 1 : this->buffer->length();
        place_caret(end, shift);
      }
    }
    e.consume();
    break;
  case KeyEvent::VK_DELETE:
    if (this->search_mode) {
      if (not this->search_pattern.empty()) {
        this->search_pattern.pop_back();
      }
      repaint_message_row();
    } else {
      delete_forward(); // deletes the selection when there is one
    }
    e.consume();
    break;
  case KeyEvent::VK_BACK_SPACE:
    if (this->search_mode) {
      if (not this->search_pattern.empty()) {
        this->search_pattern.pop_back();
      }
      repaint_message_row();
    } else {
      delete_backward();
    }
    e.consume();
    break;
  case KeyEvent::VK_INSERT:
    // Ctrl+Insert copies, Shift+Insert pastes (the Windows clipboard chords;
    // Swing's JTextComponent binds the same keys). The console claims
    // Ctrl+C, so the Edit menu relies on these where Ctrl+X/V are taken.
    if (ctrl) {
      copy();
      e.consume();
    } else if (shift) {
      paste();
      e.consume();
    }
    break;
  case KeyEvent::VK_ENTER:
    if (this->search_mode) {
      if (not this->search_pattern.empty()) {
        find_next(this->search_pattern, this->search_regexp, true);
      } else {
        this->message.clear();
      }
      this->search_mode = false;
      // The search entry row gives way to the file row beneath it, and the
      // document caret is drawn again.
      repaint_message_row();
      repaint_caret_cell();
    } else if (not ctrl) {
      insert_text("\n");
    }
    e.consume();
    break;
  case KeyEvent::VK_SPACE:
    // The terminal reports the space bar (like Enter and Tab) as a pressed
    // key, not a typed character, so it never reaches on_key_typed; insert it
    // here the way Enter is handled. In search mode the space goes into the
    // search entry (insert_text routes it there).
    insert_text(" ");
    e.consume();
    break;
  case KeyEvent::VK_TAB:
    // Same delivery as space: the parser reports Tab as a pressed key, so the
    // typed-character handler below never sees '\t'.
    insert_text("\t");
    e.consume();
    break;
  case KeyEvent::VK_ESCAPE:
    this->search_mode = false;
    repaint_message_row();
    repaint_caret_cell();
    e.consume();
    break;
  case KeyEvent::VK_F3:
    if (this->search_mode) {
      // F3 in the search entry: close it and jump to the next match of the
      // current pattern (repeat last search).
      this->search_mode = false;
      if (not this->search_pattern.empty()) {
        find_next(this->search_pattern, this->search_regexp, true);
      } else {
        this->message.clear();
        repaint_message_row();
        repaint_caret_cell();
      }
    } else {
      this->search_mode = true;
      this->search_pattern.clear();
      this->message.clear();
      repaint_message_row();
      // The document caret is not drawn while the search entry is edited.
      repaint_caret_cell();
    }
    e.consume();
    break;
  case KeyEvent::VK_F4:
    this->search_regexp = not this->search_regexp;
    show_message(std::string("regexp search ") + (this->search_regexp ? "on" : "off") + " (F3 to search)");
    e.consume();
    break;
  default:
    break;
  }
}

void TextArea::on_key_typed(KeyEvent &e) {
  auto code = e.get_key_char().get_code();
  switch (code) {
  case '\t':
    insert_text("\t");
    e.consume();
    break;
  // Control characters arrive for Ctrl+letter chords (the console delivers
  // Ctrl+A as 0x01, Ctrl+V as 0x16, ...). Ctrl+C (0x03) only reaches the app
  // on terminals that do not claim it; the canonical Copy chord is
  // Ctrl+Insert (see VK_INSERT above). In search mode a paste goes into the
  // search pattern (insert_text) instead of the document.
  case 0x01: // Ctrl+A
    select_all();
    e.consume();
    break;
  case 0x03: // Ctrl+C
    copy();
    e.consume();
    break;
  case 0x16: // Ctrl+V
    paste();
    e.consume();
    break;
  case 0x18: // Ctrl+X
    cut();
    e.consume();
    break;
  case 0x19: // Ctrl+Y
    redo();
    e.consume();
    break;
  case 0x1A: // Ctrl+Z
    undo();
    e.consume();
    break;
  default:
    if (code >= 0x20 and code != 0x7F) {
      insert_text(to_utf8(code));
      e.consume();
    }
    break;
  }
}

// ---------------------------------------------------------------------------
// search

bool TextArea::find_next(std::string const &pattern, bool regexp, bool forward) {
  auto from = this->caret;
  auto old_caret_line = this->caret_line;
  // The previous highlight (if any) is about to be replaced: erase its rows
  // first -- it may sit anywhere, not only between the old and the new caret.
  repaint_match_rows();
  if (regexp) {
    auto match = this->buffer->find_regex(pattern, from);
    if (not match and forward and from > 0) {
      match = this->buffer->find_regex(pattern, 0);
    }
    if (match) {
      this->match_start = match->first;
      this->match_end = match->second;
      this->caret = this->match_end;
      this->sel_anchor = this->caret;
      this->message.clear();
      // Index the hit so caret operations and painting near it stay cheap.
      this->buffer->ensure_scanned_to(this->match_end);
      refresh_caret_geometry();
      refresh_view_size();
      scroll_caret_to_visible();
      // The new highlight spans from the first match row to the caret; the
      // band [old caret, new caret] covers it (a match always starts at or
      // after the old caret, where the search began).
      repaint_caret_rows(old_caret_line, this->caret_line);
      restart_caret_blink();
      return true;
    }
  } else {
    auto offset = this->buffer->find(pattern, from);
    if (not offset and forward and from > 0) {
      offset = this->buffer->find(pattern, 0);
    }
    if (offset) {
      this->match_start = *offset;
      this->match_end = *offset + pattern.size();
      this->caret = this->match_end;
      this->sel_anchor = this->caret;
      this->message.clear();
      this->buffer->ensure_scanned_to(this->match_end);
      refresh_caret_geometry();
      refresh_view_size();
      scroll_caret_to_visible();
      repaint_caret_rows(old_caret_line, this->caret_line);
      restart_caret_blink();
      return true;
    }
  }
  show_message(std::string("no match for \"") + pattern + "\"");
  return false;
}

void TextArea::show_message(std::string const &message) {
  this->message = message;
  // Only the message row (the bottom visible one) changes.
  repaint_message_row();
}

// ---------------------------------------------------------------------------
// view size

void TextArea::refresh_view_size() {
  // An edit rewinds the buffer's lazy line index to the last anchor before
  // the edit (TextBuffer::replace drops the anchors there), so
  // known_line_count() can momentarily fall below what we last advertised
  // even though the document did not shrink. Re-scan back to the previous
  // frontier first, so a keystroke that keeps the line count cannot shrink
  // the content height and force the pane to re-layout and repaint its whole
  // area. ensure_line(N) guarantees at least N+1 lines are known, so passing
  // last_notified_lines-1 restores the exact previous count; a deleted newline
  // simply stops at the (shorter) content end.
  if (this->last_notified_lines > 0) {
    this->buffer->ensure_line(std::uint64_t(this->last_notified_lines) - 1);
  }
  auto lines = this->buffer->known_line_count();
  auto height = int(std::min<std::uint64_t>(lines, 1ull << 30));
  auto width = content_width();
  if (height != get_preferred_size().height or width != get_preferred_size().width or this->last_notified_lines != int64_t(lines)) {
    set_preferred_size(Dimension { width, std::max(1, height) });
    this->last_notified_lines = int64_t(lines);
    // Grow the viewport's content size immediately so scroll range and
    // clamps are right without waiting for the next layout pass, then ask
    // the pane (when there is one) to re-run its bar-visibility decision.
    if (auto viewport = get_viewport()) {
      viewport->set_view_size(std::max(1, width), std::max(1, height));
    }
    notify_window();
  }
}

int TextArea::content_width() {
  if (this->line_wrap) {
    return get_viewport() ? std::max(1, get_viewport()->get_width()) : 80;
  }
  if (this->max_line_width_stale) {
    this->max_line_width = measure_max_line_width();
    this->max_line_width_stale = false;
  }
  // One cell past the widest line: the caret at the end of that line sits in
  // a cell of its own, and the view is exactly as wide as its content --
  // without the extra cell the caret would be clipped away at the edge.
  return std::max(1, int(std::min<std::uint64_t>(this->max_line_width + 1, 1ull << 30)));
}

std::uint64_t TextArea::measure_line_cells(TextBuffer::LineRange const &range) const {
  auto offset = range.start;
  auto cell = std::uint64_t { 0 };
  while (offset < range.end) {
    auto window = this->buffer->read(offset, std::min<std::uint64_t>(4096, range.end - offset));
    auto base = offset;
    auto pos = std::size_t { 0 };
    while (pos < window.size()) {
      auto first = std::uint8_t(window[pos]);
      auto expected = first < 0x80 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
      if (window.size() - pos < std::size_t(expected)) {
        break; // partial tail: leave it for the next window
      }
      auto len = utf8_len(window.data() + pos, window.size() - pos);
      auto code = decode_char(window.data() + pos, window.size() - pos);

      // Skipped (no cell consumed): carriage returns and combining marks.
      if (code == '\r' or util::unicode::glyph_width(code) == 0) {
        pos += std::size_t(len);
        continue;
      }

      auto glyph = code;
      if (util::unicode::glyph_width(code) < 0) {
        glyph = '?';
      } else if (code == '\t') {
        glyph = ' '; // a tab is one cell in a plain viewer
      }
      auto glyph_width = util::unicode::glyph_width(glyph);
      if (glyph_width <= 0) {
        glyph = '?';
        glyph_width = 1;
      }
      cell += std::uint64_t(glyph_width);
      pos += std::size_t(len);
    }
    offset = base + pos;
    if (offset >= range.end or pos == 0) {
      break;
    }
  }
  return cell;
}

std::uint64_t TextArea::measure_max_line_width() const {
  auto total = this->buffer->scan_to_end();
  auto max = std::uint64_t { 0 };
  constexpr std::uint64_t CHUNK = 4096;
  for (std::uint64_t first = 0; first < total; first += CHUNK) {
    auto ranges = this->buffer->read_line_ranges(first, std::min(CHUNK, total - first));
    for (auto const &range : ranges) {
      max = std::max(max, measure_line_cells(range));
    }
  }
  return max;
}

void TextArea::grow_max_line_width(std::uint64_t line) {
  if (this->line_wrap or this->max_line_width_stale) {
    return; // the width is the viewport (wrap) or will be recomputed (stale)
  }
  // An edit rewinds the buffer's lazy line index to the anchor before the
  // edit (TextBuffer::replace drops the anchors there), so the edited line
  // may not be resolvable yet. ensure_line rescans from that anchor up to
  // the line -- cheap, since the anchor is just before the edit -- and makes
  // the range readable. Without it a mid-file edit never grows the content
  // width: the new cells are clipped outside the view and unreachable.
  this->buffer->ensure_line(line);
  auto ranges = this->buffer->read_line_ranges(line, 1);
  if (ranges.empty()) {
    return;
  }
  auto width = measure_line_cells(ranges[0]);
  if (width > this->max_line_width) {
    this->max_line_width = width;
  }
}

// ---------------------------------------------------------------------------
// painting

void TextArea::paint(Graphics &g) {
  auto width = get_width();
  auto height = get_height();
  if (width <= 0 or height <= 0) {
    return;
  }

  auto bg = get_background_color();
  auto fg = get_foreground_color();
  if (bg) {
    g.set_background_color(bg);
    g.fill_rect(0, 0, width, height);
  }
  g.set_foreground_color(fg);

  // The viewport clips horizontally when the view is scrolled: only the cells
  // [left, right) of each row are visible. Decoding and drawing the rest is
  // wasted work (a long line is otherwise read and decoded cell-by-cell up to
  // its whole natural width every frame).
  auto left = 0;
  auto right = width;
  if (auto viewport = get_viewport()) {
    left = std::max(0, viewport->get_view_position().x);
    right = std::min(width, left + viewport->get_width());
  }

  // Only the rows inside the viewport are visible (this view is sized to the
  // whole content); painting them is all a frame needs.
  auto visible_rows = visible_row_count();

  // The rows below the last visible one may not be in the lazy line index
  // yet; make sure the whole visible span is resolvable before drawing.
  auto top = std::uint64_t(std::max(0, get_top_line()));
  auto wanted = top + std::uint64_t(std::max(1, visible_rows));
  if (not this->buffer->is_fully_scanned() and this->buffer->known_line_count() < wanted + 1) {
    this->buffer->ensure_line(wanted + 1);
  }
  refresh_view_size();

  auto bottom_is_message = not this->message.empty() or this->search_mode;
  auto data_rows = std::max(0, visible_rows - (bottom_is_message ? 1 : 0));

  // Resolve the visible rows in one pass over the content.
  auto ranges = this->buffer->read_line_ranges(top, std::uint64_t(std::max(0, data_rows)));

  for (auto row = 0; row < data_rows; ++row) {
    if (std::size_t(row) >= ranges.size()) {
      break;
    }
    auto &range = ranges[std::size_t(row)];
    auto line = top + std::uint64_t(row);

    // The view child of the viewport sits at (-view_position) and spans the
    // whole content (see Viewport::place_view), so file line `line` lives at
    // view row `line`; only that row intersects the viewport's clip.
    auto row_y = int(line);

    // Draw the row's visible content. Only the cells up to `width` are
    // decoded; long rows are read in bounded windows so a giant single line
    // does not force a whole-file copy per frame. A character that straddles
    // a window's end is left for the next window (only fully decoded bytes
    // advance the cursor), so UTF-8 boundaries are never cut.
    auto offset = range.start;
    auto cell = 0;
    while (offset < range.end and cell < right) {
      auto want = bytes_for_cells(std::uint64_t(right) - std::uint64_t(cell));
      auto window = this->buffer->read(offset, std::min(want, range.end - offset));
      auto base = offset;
      auto pos = std::size_t(0);
      while (pos < window.size() and cell < right) {
        auto first = std::uint8_t(window[pos]);
        auto expected = first < 0x80 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
        if (window.size() - pos < std::size_t(expected)) {
          break; // partial tail: leave it for the next window
        }
        auto len = utf8_len(window.data() + pos, window.size() - pos);
        auto code = decode_char(window.data() + pos, window.size() - pos);

        // Skipped (no cell consumed): carriage returns and combining marks
        // (the cell terminal cannot attach a combining mark to the previous
        // glyph). Their bytes are consumed so offsets stay in step.
        if (code == '\r' or util::unicode::glyph_width(code) == 0) {
          pos += std::size_t(len);
          continue;
        }

        auto glyph = code;
        if (util::unicode::glyph_width(code) < 0) {
          // Control character (other than tab, handled below): show a
          // placeholder instead of letting it shift the terminal.
          glyph = '?';
        } else if (this->show_whitespace) {
          if (code == ' ') {
            glyph = 0xB7; // middle dot
          } else if (code == '\t') {
            glyph = 0xBB; // single-cell arrow-like glyph
          }
        } else if (code == '\t') {
          // A tab consumes one cell of space (no tab stops in a viewer).
          glyph = ' ';
        }
        auto glyph_width = util::unicode::glyph_width(glyph);
        if (glyph_width <= 0) {
          glyph = '?';
          glyph_width = 1;
        }

        if (cell + glyph_width > right) {
          // A wide character would straddle the right edge; leave the last
          // cell blank instead of splitting the glyph, and stop the row.
          if (glyph_width == 2 and cell == right - 1) {
            g.draw_char(Char(' '), cell, row_y);
          }
          cell = right;
          pos += std::size_t(len);
          break;
        }

        if (cell < left) {
          // Before the visible window: decode (to keep the byte offset in
          // step) but do not draw, so a horizontally scrolled long line does
          // not pay for drawing cells the viewport clips away.
          cell += glyph_width;
          pos += std::size_t(len);
          continue;
        }

        auto in_selection = is_selected(base + pos);
        auto in_match = is_match(base + pos);
        auto caret_here = this->caret >= base + pos and this->caret < base + pos + std::size_t(len);
        if (in_selection) {
          // Selected cells: Swing's JTextComponent selection colors.
          g.set_background_color(Color { 44, 62, 102 });
        } else if (in_match) {
          g.set_background_color(Color { 96, 62, 20 });
        } else {
          g.set_background_color(bg);
        }
        g.draw_char(Char(glyph), cell, row_y);
        if (caret_here and is_caret_showing()) {
          // The caret overwrites the cell it sits on: an inverse block by
          // default, an underline under the glyph in the UNDERLINE form.
          auto caret_attribute = this->caret_form == CaretForm::UNDERLINE ? Attribute::UNDERLINE : Attribute::INVERSE;
          g.draw_char(Char(glyph), cell, row_y, caret_attribute);
        }
        cell += glyph_width;
        pos += std::size_t(len);
      }
      offset = base + pos; // skip only the fully decoded bytes
      if (offset >= range.end or pos == 0) {
        break;
      }
    }

    // In Show Invisibles mode the line's terminating newline is drawn as a
    // visible symbol right after the text. `offset >= range.end` means the
    // whole line was decoded, so `cell` is its true width; the view reserves
    // one cell past the widest line for exactly this and the end-of-line
    // caret.
    //
    // The pilcrow (U+00B6), not the line-feed arrow (U+21B5): the arrow is
    // outside the Latin-1 block the other invisibles use (space = U+00B7,
    // tab = U+00BB), and Consolas -- the common Windows console font -- has
    // no glyph for it, so the console paints a box. The pilcrow is the
    // classic line-end mark (as in Notepad++'s show-all-characters) and is
    // present in every console font.
    if (this->show_whitespace and range.has_newline and offset >= range.end and cell >= left) {
      g.set_background_color(bg);
      g.draw_char(Char(char32_t(0x00B6)), cell, row_y); // the pilcrow: line end
    }

    // The caret resting at the end of its line: show the cursor right after
    // the text (only when the line's end is inside the visible horizontal
    // window and the caret is not on a character of this row).
    if (int(this->caret_line) == int(line) and this->caret_cell >= left and this->caret_cell < right) {
      if (is_caret_showing() and (this->caret >= range.end or (this->caret <= range.start and range.start == range.end))) {
        g.set_background_color(bg);
        auto caret_attribute = this->caret_form == CaretForm::UNDERLINE ? Attribute::UNDERLINE : Attribute::INVERSE;
        g.draw_char(Char(' '), this->caret_cell, row_y, caret_attribute);
      }
    }
  }

  if (bottom_is_message) {
    // The message/search row is the last *visible* row of the viewport (the
    // view row the viewport's bottom edge shows at this scroll position).
    auto row = int(top) + std::max(0, visible_rows - 1);
    auto text = this->search_mode
        ? (this->search_regexp ? "search [regexp]: " : "search: ") + this->search_pattern + "_"
        : this->message;
    if (int(text.size()) > width) {
      text.resize(std::size_t(width));
    }
    g.set_background_color(bg);
    g.set_foreground_color(Color { 240, 200, 90 });
    g.fill_rect(0, row, width, 1);
    g.draw_string(text, 0, row);
    g.set_foreground_color(fg);
  }
}

}
