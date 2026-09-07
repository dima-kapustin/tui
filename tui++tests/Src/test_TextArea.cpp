// Unit tests for the TextArea editing operations: selection, clipboard
// cut/copy/paste, select-all and undo/redo. The key plumbing (focus, window
// forwarders) needs a shown window, so the operations are exercised through
// their public entry points.

#include <tui++/Clipboard.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>

#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

using namespace tui;

namespace {

std::shared_ptr<TextArea> make_area(std::string const &content) {
  auto buffer = TextBuffer::create_empty();
  buffer->replace(0, 0, content);
  buffer->scan_to_end();
  auto area = make_component<TextArea>();
  area->set_buffer(buffer);
  return area;
}

std::string content_of(TextArea const &area) {
  return area.get_buffer()->read(0, area.get_buffer()->length());
}

void test_clipboard_roundtrip() {
  Clipboard::set_text("clip one");
  assert(Clipboard::has_text());
  assert(Clipboard::get_text() == "clip one");
  Clipboard::set_text("");
  assert(not Clipboard::has_text());
}

void test_paste_inserts_at_caret() {
  auto area = make_area("hello\n");
  Clipboard::set_text("XY");
  area->paste();
  assert(content_of(*area) == "XYhello\n");
  assert(area->get_caret() == 2); // after the pasted text

  // Pasting in the middle moves the tail along.
  area->set_caret(5);
  area->paste();
  assert(content_of(*area) == "XYhelXYlo\n");
  assert(area->get_caret() == 7);
}

void test_select_all_and_copy() {
  auto area = make_area("first line\nsecond\n");
  area->select_all();
  assert(area->has_selection());
  auto [start, end] = area->get_selection();
  assert(start == 0 and end == 18);
  area->copy();
  assert(Clipboard::get_text() == "first line\nsecond\n");

  // A plain caret move collapses the selection.
  area->set_caret(3);
  assert(not area->has_selection());
}

void test_cut_deletes_selection() {
  auto area = make_area("cut me out\n");
  Clipboard::set_text("old");
  area->select_all();
  area->cut();
  assert(content_of(*area).empty());
  assert(Clipboard::get_text() == "cut me out\n");

  // Without a selection, cut copies nothing and deletes nothing.
  Clipboard::set_text("keep");
  area->cut();
  assert(content_of(*area).empty());
  assert(Clipboard::get_text() == "keep");

  // Cut restores the surrounding text on undo.
  area->undo();
  assert(content_of(*area) == "cut me out\n");
}

void test_delete_verbs_with_selection() {
  auto area = make_area("0123456789");
  // Forward delete of a selected range.
  area->select_all();
  area->delete_forward();
  assert(content_of(*area).empty());
  assert(area->can_undo());
  area->undo();
  assert(content_of(*area) == "0123456789");

  // Backward delete without selection removes the char before the caret.
  area->set_caret(4);
  area->delete_backward(); // removes the '3' at index 3
  assert(content_of(*area) == "012456789");
  assert(area->get_caret() == 3);
  area->undo();
  assert(content_of(*area) == "0123456789");
}

void test_typing_run_coalesces() {
  // Typing is exercised through paste only here (no key plumbing without a
  // window), so verify the undo-step bookkeeping via several pastes at a
  // growing caret, which mimics a typing run: one undo removes them all.
  auto area = make_area("");
  Clipboard::set_text("a");
  area->paste();
  area->paste();
  Clipboard::set_text("b");
  area->paste();
  assert(content_of(*area) == "aab");
  assert(area->can_undo());
  area->undo();
  assert(content_of(*area).empty()); // one coalesced step
  assert(not area->can_undo());
}

void test_undo_redo() {
  auto area = make_area("base\n");
  Clipboard::set_text("inserted ");
  area->set_caret(4); // on the newline: the insert lands before it
  area->paste();
  assert(content_of(*area) == "baseinserted \n");
  assert(area->get_caret() == 13); // 4 + "inserted "

  area->undo();
  assert(content_of(*area) == "base\n");
  assert(area->can_redo());

  area->redo();
  assert(content_of(*area) == "baseinserted \n");
  assert(area->get_caret() == 13);

  // A fresh edit after an undo drops the redo history (Swing behavior).
  area->undo();
  assert(area->can_redo());
  Clipboard::set_text("x");
  area->paste();
  assert(content_of(*area) == "basex\n");
  assert(not area->can_redo());
}

void test_readonly() {
  auto area = make_area("read only text");
  area->set_readonly(true);
  Clipboard::set_text("XX");
  auto before = content_of(*area);
  area->paste();
  assert(content_of(*area) == before); // nothing changed
  area->select_all();
  area->cut();
  assert(content_of(*area) == before);
  // Copy still works on a read-only area.
  assert(Clipboard::get_text() == "read only text");
  area->undo();
  assert(content_of(*area) == before);
}

void test_huge_edits_leave_no_history() {
  auto area = make_area("");
  Clipboard::set_text(std::string(1 << 20, 'x')); // 1 MiB paste
  area->paste();
  assert(content_of(*area).size() == std::size_t(1) << 20);
  assert(not area->can_undo()); // too big to record
}

} // namespace

void test_TextArea_editing() {
  test_clipboard_roundtrip();
  test_paste_inserts_at_caret();
  test_select_all_and_copy();
  test_cut_deletes_selection();
  test_delete_verbs_with_selection();
  test_typing_run_coalesces();
  test_undo_redo();
  test_readonly();
  test_huge_edits_leave_no_history();
  std::fprintf(stderr, "test_TextArea_editing: ok\n");
}
