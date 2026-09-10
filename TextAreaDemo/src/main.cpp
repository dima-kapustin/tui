// TextArea demo for tui++.
//
// A standalone executable that exercises the JTextArea-style TextArea of tui++
// on the text backend:
//
//     TextAreaDemo                     open an in-memory demo document
//     TextAreaDemo <file>              open a (potentially huge) file
//     TextAreaDemo --log-events [...]  ... with the event/resize log on
//
// The window hosts ONE ScrollPane whose viewport shows a TextArea bound to a
// TextBuffer. The buffer memory-maps the file (no whole-file load), indexes
// lines lazily and copies only edited pages, so files in the 10s-of-GB range
// can be viewed, edited and searched with bounded memory.
//
// A Swing-style menu bar sits on top: File (Exit) and Edit (Undo/Redo,
// Cut/Copy/Paste, Delete, Select All, Column Select Mode, Show Invisibles).
// The Edit shortcuts live in the item's accelerator column, right-aligned as
// in Swing's menu layout. Selection:
// Shift+arrows / Shift+click / mouse drags select arbitrary text; Alt+drag
// and Alt+Shift+arrows select a column block, and F8 (Edit > Column Select
// Mode) makes every selection gesture select columns -- Alt+Shift is taken
// by the Windows input-language switch on some hosts, so the mode brings
// column selection to them. Chords: Ctrl+X cut, Ctrl+Insert copy, Ctrl+V
// paste, Ctrl+A select all, Ctrl+Z / Ctrl+Y undo/redo (the console keeps
// Ctrl+C, so Copy uses Swing's secondary Ctrl+Insert binding). Typing,
// paste and delete replace the selection, a column selection included.
// Caret navigation: arrows move by character/row, Ctrl+Left/Right jump to
// the start/end of the line, Ctrl+Up/Down page up/down (PageUp/PageDown do
// the same), Home/End to the line's edges, Ctrl+Home/End to the document's.
// F3 search, F4 regexp search, F5 Show Invisibles (whitespace dots/pilcrows),
// F6 caret form (block/underline), F7 caret blink (blink/steady/hidden),
// F8 column select mode. The status line mirrors the buffer and caret state.
//
// Find All (F9, or Edit > Find All) lists every non-overlapping occurrence
// of the area's current search pattern (the one its F3 search entry holds)
// in a results pane between the file view and the status line. Each hit is
// one row, "line:column  <text of the source line>"; clicking a row or
// moving the caret in the list (arrows, PageUp/Down, Home/End) previews the
// hit in the file view, Enter previews and closes the panel, Esc closes it,
// F9 re-runs the search. The scan is byte-accurate and safe for UTF-8 (it
// never decodes), windowed (bounded memory on huge files) and capped at
// FIND_ALL_LIMIT hits; regexp find-all is not built yet, so the list is
// always plain-substring.
//
// Quit with Ctrl+C or the File menu.

#include <tui++/BorderLayout.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Graphics.h>
#include <tui++/KeyStroke.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Screen.h>
#include <tui++/ScrollPane.h>
#include <tui++/TextArea.h>
#include <tui++/TextBuffer.h>
#include <tui++/Viewport.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/util/log.h>

#include <tui++/Panel.h>
#include <tui++/util/utf-8.h>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace tui;

namespace {

// Find All (F9 / Edit > Find All): the largest hit list kept in the results
// pane, and the cap (in bytes) of the source-line preview each row carries.
// The list is capped so a search over a huge file scans at most until that
// many hits and the results buffer stays small; the status line reports when
// the cap cut the list short.
constexpr std::size_t FIND_ALL_LIMIT = 2000;
constexpr std::size_t FIND_ALL_PREVIEW_BYTES = 200;

// A small in-memory demo document used when no file argument is given: it has
// enough lines to make scrolling, paging and search worth trying, and its
// last line repeats a phrase so searches find several matches.
std::shared_ptr<TextBuffer> make_demo_buffer() {
  auto buffer = TextBuffer::create_empty();
  auto text = std::string { };
  text.reserve(std::size_t(6) << 20);
  for (auto i = 0; i < 4000; ++i) {
    if (i % 97 == 0) {
      text += "The quick brown fox jumps over the lazy dog and keeps on jumping.\n";
    } else {
      char line[128];
      std::snprintf(line, sizeof line, "demo line %04d with some text to look at while scrolling and searching\n", i);
      text += line;
    }
  }
  text += "end of demo document (type below to edit; F3 to search, F5 to show whitespace)\n";
  buffer->replace(0, 0, text);
  return buffer;
}

// The status line under the pane: file/buffer state and caret position. It
// repaints only when the reported text actually changed.
class StatusLine: public Component {
public:
  std::function<std::string()> text;

  void refresh() {
    auto current = this->text ? this->text() : std::string { };
    if (current != this->last) {
      this->last = std::move(current);
      repaint();
    }
  }

protected:
  void paint(Graphics &g) override {
    auto bg = get_background_color();
    auto fg = get_foreground_color();
    if (bg) {
      g.set_background_color(bg);
      g.fill_rect(0, 0, get_width(), get_height());
    }
    g.set_foreground_color(fg);
    auto text = this->last;
    if (int(text.size()) > get_width()) {
      text.resize(std::size_t(std::max(0, get_width())));
    }
    g.draw_string(text, 0, 0);
  }

private:
  std::string last;
};

// Refreshes the status line after every key event of the window (registered
// after the TextArea's own key forwarder, which is added at show time).
class KeyRefresher final: public EventListener<KeyEvent> {
  std::function<void()> refresh;

public:
  explicit KeyRefresher(std::function<void()> refresh) :
      refresh(std::move(refresh)) {
  }
  void key_pressed(KeyEvent &) override {
    this->refresh();
  }
  void key_typed(KeyEvent &) override {
    this->refresh();
  }
};

struct FindHit {
  std::uint64_t offset;  // where the hit starts in the source buffer
  std::uint64_t line;    // its line, and the byte column of `offset` on it
  std::uint64_t column;
};

struct DemoState {
  std::shared_ptr<ScrollPane> pane;
  std::shared_ptr<TextArea> area;
  std::shared_ptr<StatusLine> status;

  // Find All state: the results pane between the file view and the status
  // line, and the read-only TextArea listing one hit per row (buffer row r
  // corresponds to hits[r]; see run_find_all).
  std::shared_ptr<ScrollPane> results_pane;
  std::shared_ptr<TextArea> results_area;
  std::vector<FindHit> hits;
  bool find_capped = false;           // the FIND_ALL_LIMIT cut the list short
  std::size_t previewed = SIZE_MAX;   // the hit last previewed in the file view
};

// Menu helpers, following the MenuBarDemo wiring: an item's pick closes its
// popup and runs the action; a click on a top-level menu opens its popup and
// closes every other menu's (see wire_menu_popup_toggle). An optional
// `accelerator` (Swing's JMenuItem.setAccelerator) gives the item a shortcut
// shown right-aligned in the popup's accelerator column.
std::shared_ptr<MenuItem> add_item(const std::shared_ptr<Menu> &menu, std::string text, char mnemonic, std::optional<KeyStroke> const &accelerator, std::function<void()> action) {
  auto item = mnemonic ? make_component<MenuItem>(std::move(text), Char { mnemonic }) : make_component<MenuItem>(std::move(text));
  if (accelerator) {
    item->set_accelerator(accelerator.value());
  }
  auto weak_menu = std::weak_ptr<Menu> { menu };
  item->add_listener([weak_menu, action = std::move(action)](ActionEvent &) {
    if (auto menu = weak_menu.lock()) {
      menu->set_popup_menu_visible(false);
    }
    action();
  });
  menu->add(item);
  return item;
}

// A click on a top-level menu opens its popup (closing every other menu's); a
// re-click of a menu whose popup this wiring opened with an earlier click
// closes it again. The bar's hover behavior complicates the toggle: gliding
// over another top-level menu while a popup is open switches the open popup
// to it (as in Swing's menu bar), so by the time the pointer reaches a menu
// the user is about to click, that menu's popup may already be showing -- and
// a click must not close what the hover just opened, or clicking a menu the
// pointer crossed could never display its popup. Only a popup opened by a
// click counts as "open" for the toggle; the click that follows a hover-open
// keeps the popup. Any other dismissal (picking an item, clicking the
// content) clears the flag through the popup's BECOMES_INVISIBLE event.
void wire_menu_popup_toggle(const std::shared_ptr<Menu> &menu, const std::initializer_list<std::shared_ptr<Menu>> &others) {
  auto weak_self = std::weak_ptr<Menu> { menu };
  auto weak_others = std::vector<std::weak_ptr<Menu>> { };
  weak_others.reserve(others.size());
  for (auto &&other : others) {
    weak_others.emplace_back(other);
  }
  auto clicked_open = std::make_shared<bool>(false);
  if (auto popup_menu = menu->get_popup_menu()) {
    popup_menu->add_listener([clicked_open](PopupMenuEvent &e) {
      if (e.id == PopupMenuEvent::BECOMES_INVISIBLE) {
        *clicked_open = false;
      }
    });
  }
  menu->add_listener([weak_self, weak_others, clicked_open](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_RELEASED) {
      return;
    }
    auto self = weak_self.lock();
    if (not self) {
      return;
    }
    if (self->is_popup_menu_visible() and *clicked_open) {
      // The deliberate re-click of the menu this wiring opened: close it.
      self->set_popup_menu_visible(false);
    } else if (not self->is_popup_menu_visible()) {
      for (auto &&weak_other : weak_others) {
        if (auto other = weak_other.lock()) {
          other->set_popup_menu_visible(false);
        }
      }
      self->set_popup_menu_visible(true);
      *clicked_open = true;
    }
    // A popup the hover-switch already opened stays open: the click confirms
    // the menu the pointer settled on.
    self->set_armed(true);
    self->repaint();
    e.consume();
  });
}

std::string status_text(DemoState const &state) {
  auto area = state.area;
  auto buffer = area->get_buffer();
  auto viewport = state.pane->get_viewport();
  auto position = viewport->get_view_position();
  auto v_model = state.pane->get_vertical_scroll_bar()->get_model();
  auto caret = area->get_caret();
  auto length = buffer->length();
  auto [line, column] = buffer->offset_to_line(caret);
  char buffer_text[320];
  std::snprintf(buffer_text, sizeof buffer_text,
                "row=%d  lines=%llu%s  caret=%llu (%llu:%llu)  bytes=%llu  ws=%s  regexp=%s%s%s",
                position.y,
                static_cast<unsigned long long>(buffer->known_line_count()),
                buffer->is_fully_scanned() ? " (scanned)" : "",
                static_cast<unsigned long long>(caret),
                static_cast<unsigned long long>(line),
                static_cast<unsigned long long>(column),
                static_cast<unsigned long long>(length),
                area->is_show_whitespace() ? "on" : "off",
                area->is_search_regexp() ? "on" : "off",
                area->is_column_select_mode() ? "  colmode=on" : "",
                area->is_block_selection() ? "  block" : "");
  if (state.results_pane and state.results_pane->is_visible()) {
    auto used = std::strlen(buffer_text);
    std::snprintf(buffer_text + used, sizeof buffer_text - used, "  hits=%llu%s",
                  static_cast<unsigned long long>(state.hits.size()),
                  state.find_capped ? " (capped)" : "");
  }
  return buffer_text;
}

// The longest prefix of `text` of at most `max_bytes` bytes that ends on a
// UTF-8 character boundary, so a preview cut mid-sequence never lets a
// partial multi-byte character into the results list.
std::string_view utf8_capped(std::string_view text, std::size_t max_bytes) {
  auto cap = std::min(max_bytes, text.size());
  if (cap == 0) {
    return {};
  }
  // Walk back from the cut over continuation bytes to the head of the
  // character the last kept byte belongs to.
  auto start = cap - 1;
  while (start > 0 and (std::uint8_t(text[start]) & 0xC0) == 0x80) {
    --start;
  }
  if ((std::uint8_t(text[start]) & 0xC0) == 0x80) {
    return {}; // the kept prefix begins inside a sequence: nothing to show
  }
  auto const head_len = std::size_t(util::utf8_sequence_length(std::uint8_t(text[start])));
  return head_len <= cap - start ? text.substr(0, cap) : text.substr(0, start);
}

// Jumps the file view's caret to hit `index` of the results list (no-op for
// a marker row beyond the hits, or when the hit is already previewed).
void preview_result(DemoState &state, std::size_t index) {
  if (index >= state.hits.size() or index == state.previewed) {
    return;
  }
  state.previewed = index;
  // set_caret indexes the hit's surroundings, scrolls the row into view and
  // repaints; the caret is drawn once the file area regains the focus (after
  // Enter or a click there).
  state.area->set_caret(state.hits[index].offset);
  state.status->refresh();
}

// The buffer row the results area's caret sits on (every hit is one row).
std::size_t results_caret_row(DemoState const &state) {
  auto results = state.results_area;
  return std::size_t(results->get_buffer()->offset_to_line(results->get_caret()).first);
}

// Hides the results pane and returns the keyboard focus to the file view.
void close_results(DemoState &state) {
  if (state.results_pane->is_visible()) {
    state.results_pane->set_visible(false);
    state.results_pane->revalidate();
  }
  state.area->request_input_focus();
  state.status->refresh();
}

// Runs the find-all search and (re)opens the results pane. The pattern is
// the file area's current search pattern (see TextArea::get_search_pattern):
// what its F3 entry holds, which survives the entry closing, so F9 repeats
// the last search. Every hit becomes one row of a fresh in-memory buffer
// shown by the read-only results area; the row text is the source line's
// first bytes (capped at a character boundary), so a log line tens of
// megabytes long cannot bloat the results buffer.
void run_find_all(DemoState &state) {
  auto area = state.area;
  auto pattern = area->get_search_pattern();
  if (pattern.empty()) {
    // While the F3 entry is open the message row shows the entry, so only
    // nag when there is no entry to type in.
    if (not area->is_search_mode()) {
      area->show_message("no pattern yet: F3, type a search, then F9 for Find All");
    }
    return;
  }
  if (area->is_search_mode()) {
    // Commit the entry: F9 finds all occurrences of the pattern typed so
    // far (the pattern stays, so the entry opens clean the next time).
    area->close_search_entry();
  }
  auto source = area->get_buffer();
  // Ask for one hit more than the cap so "capped" is exact (the list holds
  // FIND_ALL_LIMIT hits either way).
  auto offsets = source->find_all(pattern, 0, FIND_ALL_LIMIT + 1);
  state.find_capped = offsets.size() > FIND_ALL_LIMIT;
  if (state.find_capped) {
    offsets.resize(FIND_ALL_LIMIT);
  }

  state.hits.clear();
  state.hits.reserve(offsets.size());
  auto rows = std::string();
  rows.reserve(offsets.size() * 96);
  for (auto offset : offsets) {
    // The hits come sorted; the per-hit ensure_scanned_to is one forward
    // pass, and each hit's line then resolves without rescanning.
    source->ensure_scanned_to(offset);
    auto [line, column] = source->offset_to_line(offset);
    state.hits.push_back({ offset, line, column });
    char head[32];
    std::snprintf(head, sizeof head, "%10llu:%-6llu ",
                  static_cast<unsigned long long>(line),
                  static_cast<unsigned long long>(column));
    rows += head;
    auto ranges = source->read_line_ranges(line, 1);
    if (not ranges.empty()) {
      // end points at the terminating '\n', so the content length is
      // end - start already (no newline byte to subtract).
      auto text_len = ranges[0].end - ranges[0].start;
      auto take = std::min<std::uint64_t>(text_len, FIND_ALL_PREVIEW_BYTES + 4);
      if (take > 0) {
        auto text = source->read(ranges[0].start, take);
        if (not text.empty() and text.back() == '\r') {
          text.pop_back(); // CRLF files: keep the preview clean
        }
        rows += utf8_capped(text, FIND_ALL_PREVIEW_BYTES);
      }
    }
    rows += '\n';
  }
  if (offsets.empty()) {
    rows += "(no matches for \"" + pattern + "\")\n";
  } else if (state.find_capped) {
    rows += "(more hits than FIND_ALL_LIMIT: refine the pattern)\n";
  }

  auto results_buffer = TextBuffer::create_empty();
  results_buffer->replace(0, 0, rows);
  state.results_area->set_buffer(results_buffer);
  state.previewed = SIZE_MAX;
  auto results_pane = state.results_pane;
  if (not results_pane->is_visible()) {
    results_pane->set_visible(true);
  }
  results_pane->revalidate();
  results_pane->get_viewport()->set_view_position(0, 0);
  state.results_area->request_input_focus();
  state.status->refresh();
}

std::shared_ptr<Frame> build_text_area_demo(std::shared_ptr<TextBuffer> const &buffer) {
  auto state = std::make_shared<DemoState>();

  // The whitespace rendering toggle, shared by the F5 key and the Edit menu's
  // "Show Invisibles" item (Swing leaves such view options to the
  // application; TextArea itself binds no key for it).
  auto toggle_invisibles = [](std::shared_ptr<TextArea> const &area) {
    area->set_show_whitespace(not area->is_show_whitespace());
    area->show_message(std::string("whitespace ") + (area->is_show_whitespace() ? "visible" : "hidden"));
  };

  // The column-select mode toggle, shared by the F8 key and the Edit menu's
  // "Column Select Mode" item. While on, every selection gesture (drag,
  // Shift+click, Shift+arrows) selects a column block; Alt+drag and
  // Alt+Shift+arrows select a block either way (they are the mode's escape
  // hatch on hosts where Alt+Shift is taken, e.g. the Windows input-language
  // switch).
  auto toggle_column_mode = [](std::shared_ptr<TextArea> const &area) {
    area->set_column_select_mode(not area->is_column_select_mode());
    area->show_message(area->is_column_select_mode()
        ? "column select mode on: drags and Shift select columns (F8 off)"
        : "column select mode off (Alt+drag still selects a column)");
  };

  auto frame = make_component<Frame>();
  frame->set_background_color(Color { 14, 16, 24 });
  frame->set_size(screen.get_size());
  frame->set_name("text area demo frame");

  auto pane = make_component<ScrollPane>();
  state->pane = pane;
  pane->set_name("text scroll pane");
  pane->set_background_color(Color { 10, 12, 18 });
  frame->add(pane);

  auto area = make_component<TextArea>();
  state->area = area;
  area->set_buffer(buffer);
  pane->set_viewport_view(area);

  // The status line below the pane. Refreshed when the scroll bar models
  // move and after every key/mouse event that reaches the window. It sits in
  // the bottom strip under the find-all results pane (which is hidden until
  // the first F9), so the strip is one row tall without results and grows to
  // the results pane's 10 rows plus 1 with them; the file pane above shrinks
  // accordingly.
  auto status = make_component<StatusLine>();
  state->status = status;
  status->set_preferred_size(Dimension { 0, 1 });
  status->set_background_color(Color { 8, 10, 15 });
  status->set_foreground_color(Color { 130, 140, 160 });
  status->set_name("status line");
  status->text = [state] {
    return status_text(*state);
  };
  auto refresh = [status] {
    status->refresh();
  };
  pane->get_vertical_scroll_bar()->get_model()->add_change_listener(refresh);
  pane->get_horizontal_scroll_bar()->get_model()->add_change_listener(refresh);

  // The find-all results pane: a read-only TextArea listing one hit per row
  // (see run_find_all). It is created hidden; Find All (F9) shows it between
  // the file view and the status line, Esc (or Enter after a jump) hides it.
  auto results_pane = make_component<ScrollPane>();
  state->results_pane = results_pane;
  results_pane->set_name("find all results");
  results_pane->set_preferred_size(Dimension { 0, 10 });
  results_pane->set_background_color(Color { 10, 12, 18 });
  results_pane->set_visible(false);
  auto results_area = make_component<TextArea>();
  state->results_area = results_area;
  results_area->set_name("results list");
  results_area->set_readonly(true);
  // Long source lines are cut at the pane's right edge instead of widening
  // the list (each hit stays one row).
  results_area->set_line_wrap(true);
  results_area->set_background_color(Color { 10, 12, 18 });
  results_area->set_foreground_color(Color { 148, 158, 178 });
  results_area->set_buffer(TextBuffer::create_empty());
  results_pane->set_viewport_view(results_area);
  // A click on a result row previews the hit in the file view. The area's
  // own press handler (registered at init, before this one) has already
  // placed the caret on the clicked row.
  results_area->add_listener([state](MousePressEvent &e) {
    if (e.id == MousePressEvent::MOUSE_PRESSED) {
      preview_result(*state, std::size_t(std::max(0, e.y)));
    }
  });

  auto bottom = make_component<Panel>();
  bottom->set_name("results and status");
  bottom->set_layout(std::make_shared<BorderLayout>());
  bottom->add(results_pane);
  bottom->add(status, BorderLayout::SOUTH);
  frame->add(bottom, BorderLayout::SOUTH);
  refresh();

  // The menu bar: File (exit) and Edit (the editing operations of the text
  // area). Picking an Edit item gives the focus back to the area so typing
  // continues where the menu left off.
  auto weak_area = std::weak_ptr<TextArea> { area };
  auto edit_action = [weak_area, refresh](std::function<void(std::shared_ptr<TextArea> const &)> const &operation) {
    return [weak_area, refresh, operation] {
      if (auto area = weak_area.lock()) {
        operation(area);
        refresh();
        area->request_input_focus();
      }
    };
  };

  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  auto exit_item = add_item(file_menu, "Exit", 'x', std::nullopt, [] {
    terminal.shutdown();
  });
  (void)exit_item;

  auto edit_menu = make_component<Menu>("Edit");
  edit_menu->set_mnemonic('E');
  // Find All opens the results pane for the area's current search pattern
  // (the F3 entry's). The F9 shortcut is handled by the window key handler
  // below, the way F5/F8 accelerators are.
  auto find_all_item = add_item(edit_menu, "Find All", 'F', KeyStroke { KeyEvent::VK_F9, InputEvent::NO_MODIFIERS }, [state] {
    run_find_all(*state);
  });
  edit_menu->add_separator();
  auto undo_item = add_item(edit_menu, "Undo", 'U', KeyStroke { KeyEvent::VK_Z, InputEvent::CTRL_DOWN }, edit_action([](auto const &area) {
    area->undo();
  }));
  auto redo_item = add_item(edit_menu, "Redo", 'R', KeyStroke { KeyEvent::VK_Y, InputEvent::CTRL_DOWN }, edit_action([](auto const &area) {
    area->redo();
  }));
  edit_menu->add_separator();
  auto cut_item = add_item(edit_menu, "Cut", 't', KeyStroke { KeyEvent::VK_X, InputEvent::CTRL_DOWN }, edit_action([](auto const &area) {
    area->cut();
  }));
  auto copy_item = add_item(edit_menu, "Copy", 'C', KeyStroke { KeyEvent::VK_INSERT, InputEvent::CTRL_DOWN }, edit_action([](auto const &area) {
    area->copy();
  }));
  auto paste_item = add_item(edit_menu, "Paste", 'P', KeyStroke { KeyEvent::VK_V, InputEvent::CTRL_DOWN }, edit_action([](auto const &area) {
    area->paste();
  }));
  auto delete_item = add_item(edit_menu, "Delete", 'D', std::nullopt, edit_action([](auto const &area) {
    // Deletes the selection when there is one (delete_forward does), else
    // the character after the caret.
    area->delete_forward();
  }));
  edit_menu->add_separator();
  auto select_all_item = add_item(edit_menu, "Select All", 'A', KeyStroke { KeyEvent::VK_A, InputEvent::CTRL_DOWN }, edit_action([](auto const &area) {
    area->select_all();
  }));
  edit_menu->add_separator();
  // The column-select mode (see the F8 case of the key handler below). The
  // item is a plain toggle like Show Invisibles: the mode shows up in the
  // status line and the message row reports the current state.
  auto column_mode_item = add_item(edit_menu, "Column Select Mode", 'M', KeyStroke { KeyEvent::VK_F8, InputEvent::NO_MODIFIERS }, edit_action([toggle_column_mode](auto const &area) {
    toggle_column_mode(area);
  }));
  edit_menu->add_separator();
  // The view toggle (Swing leaves this to the application, so the shortcut
  // is the item's accelerator rather than a key handler inside TextArea).
  auto invisibles_item = add_item(edit_menu, "Show Invisibles", 'I', KeyStroke { KeyEvent::VK_F5, InputEvent::NO_MODIFIERS }, edit_action([toggle_invisibles](auto const &area) {
    toggle_invisibles(area);
  }));
  (void)undo_item;
  (void)redo_item;
  (void)cut_item;
  (void)copy_item;
  (void)paste_item;
  (void)delete_item;
  (void)select_all_item;
  (void)column_mode_item;
  (void)invisibles_item;
  (void)find_all_item;

  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(file_menu);
  menu_bar->add(edit_menu);
  frame->set_menu_bar(menu_bar);
  menu_bar->set_name("menu bar");
  wire_menu_popup_toggle(file_menu, { edit_menu });
  wire_menu_popup_toggle(edit_menu, { file_menu });

  // A click on the content (outside the open popup) dismisses the menu:
  // both text areas are "the content".
  auto dismiss_menus = [file_menu, edit_menu](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    for (auto &&menu : { file_menu, edit_menu }) {
      if (menu->is_popup_menu_visible()) {
        menu->set_popup_menu_visible(false);
      }
    }
  };
  area->add_listener(dismiss_menus);
  results_area->add_listener(dismiss_menus);

  // The caret/scroll changes caused by mouse presses and releases in the
  // area (a release ends a drag selection, whose caret the status mirrors).
  area->add_listener([refresh](MousePressEvent &e) {
    if (e.id == MousePressEvent::MOUSE_PRESSED or e.id == MousePressEvent::MOUSE_RELEASED) {
      refresh();
    }
  });

  frame->set_visible(true);
  frame->add_listener(std::make_shared<KeyRefresher>(refresh));
  // The caret's look is configurable, as a Swing text component's caret is:
  // F6 cycles the form (block/underline) and F7 the blink mode
  // (blinking/steady/hidden), reported on the message row. The keys the menu
  // items already carry as accelerators (F5 for Show Invisibles, F8 for
  // Column Select Mode, F9 for Find All) are left to them.
  frame->add_listener([area, refresh](KeyEvent &e) {
    if (e.id != KeyEvent::KEY_PRESSED) {
      return;
    }
    auto changed = false;
    switch (e.get_key_code()) {
    case KeyEvent::VK_F6:
      area->set_caret_form(area->get_caret_form() == TextArea::CaretForm::BLOCK ? TextArea::CaretForm::UNDERLINE : TextArea::CaretForm::BLOCK);
      changed = true;
      e.consume();
      break;
    case KeyEvent::VK_F7: {
      using namespace std::chrono;
      auto rate = area->get_caret_blink_rate();
      if (rate > milliseconds::zero()) {
        // blinking -> steady (always shown)
        area->set_caret_blink_rate(milliseconds::zero());
      } else if (area->is_caret_visible()) {
        // steady -> hidden
        area->set_caret_visible(false);
      } else {
        // hidden -> blinking again
        area->set_caret_visible(true);
        area->set_caret_blink_rate(milliseconds(530));
      }
      changed = true;
      e.consume();
      break;
    }
    default:
      break;
    }
    if (changed) {
      auto describe = [area] {
        auto form = area->get_caret_form() == TextArea::CaretForm::UNDERLINE ? "underline" : "block";
        if (not area->is_caret_visible()) {
          return std::string("caret hidden (F7 to show)");
        }
        auto rate = area->get_caret_blink_rate();
        auto blink = rate > std::chrono::milliseconds::zero()
            ? "blinking " + std::to_string(rate.count()) + " ms (F7 steady)"
            : "steady (F7 hidden)";
        return std::string("caret ") + form + ", " + blink;
      };
      area->show_message(describe());
      refresh();
    }
  });
  // Find All keyboard handling: the panel's own navigation. F9 runs (or
  // re-runs) the search through its menu item's accelerator; a click or the
  // caret keys in the results area move its caret first (this listener runs
  // after the area's own key forwarder), so the row the caret landed on is
  // previewed in the file view; Enter previews and closes the panel; Esc
  // closes it from either text area.
  frame->add_listener([state](KeyEvent &e) {
    if (e.id != KeyEvent::KEY_PRESSED) {
      return;
    }
    auto results = state->results_area;
    auto results_open = state->results_pane->is_visible();
    if (not results_open) {
      return;
    }
    auto row = results_caret_row(*state);
    if (not results->is_focus_owner()) {
      // The file area (or nothing) owns the focus: Esc dismisses the panel.
      if (e.get_key_code() == KeyEvent::VK_ESCAPE) {
        close_results(*state);
      }
      return;
    }
    switch (e.get_key_code()) {
    case KeyEvent::VK_UP:
    case KeyEvent::VK_DOWN:
    case KeyEvent::VK_LEFT:
    case KeyEvent::VK_RIGHT:
    case KeyEvent::VK_PAGE_UP:
    case KeyEvent::VK_PAGE_DOWN:
    case KeyEvent::VK_HOME:
    case KeyEvent::VK_END:
      // The results area already moved its caret to the row under the key.
      preview_result(*state, row);
      break;
    case KeyEvent::VK_ENTER:
      preview_result(*state, row);
      close_results(*state);
      break;
    case KeyEvent::VK_ESCAPE:
      close_results(*state);
      break;
    default:
      break;
    }
  });
  // The area becomes the keyboard focus owner once the window is shown.
  area->request_input_focus();
  return frame;
}

int usage(const char *program) {
  std::fprintf(stderr,
               "usage: %s [--log-events] [file]\n"
               "\n"
               "TextArea demo for tui++ (text screen). With no file argument an\n"
               "in-memory demo document is opened. A File/Edit menu bar and the\n"
               "text area implement Swing-style editing: drags select arbitrary\n"
               "text, Shift+arrows / Shift+click extend, Alt+drag and\n"
               "Alt+Shift+arrows select a column block (F8 column select mode\n"
               "makes every selection gesture select columns), Ctrl+X cut,\n"
               "Ctrl+Insert copy, Ctrl+V paste, Ctrl+A select all, Ctrl+Z /\n"
               "Ctrl+Y undo/redo (the shortcuts are the menu items'\n"
               "accelerators, right-aligned in the Edit popup); typing, paste\n"
               "and delete replace the selection, a column selection included.\n"
               "Caret navigation: arrows move by character/row, Ctrl+Left/Right\n"
               "jump to the line's start/end, Ctrl+Up/Down page up/down,\n"
               "Home/End to the line's edges, Ctrl+Home/End to the document's.\n"
               "F3 search (Enter jumps, F3 repeats), F4 regexp search, F5 Show\n"
               "Invisibles (the Edit menu's whitespace toggle), F6 caret form\n"
               "(block/underline), F7 caret blink (blinking/steady/hidden), F8\n"
               "column select mode. F9 Find All (Edit > Find All) lists every\n"
               "hit of the last search pattern in a results pane: click a row or\n"
               "use the caret keys to preview the hit in the file view, Enter\n"
               "previews and closes the pane, Esc closes it, F9 re-runs the\n"
               "search (the list is plain-substring, UTF-8 safe and capped).\n"
               "--log-events writes the event/resize/graphics history to stderr.\n",
               program);
  return 1;
}

} // namespace

int main(int argc, char *argv[]) {
  auto trace = false;
  auto path = std::string { };
  for (auto i = 1; i < argc; ++i) {
    auto arg = std::string_view { argv[i] };
    if (arg == "--log-events" or arg == "-v") {
      trace = true;
    } else if (path.empty()) {
      path = arg;
    } else {
      return usage(argv[0]);
    }
  }

  if (trace) {
    util::event_log = &std::cerr;
    log_event_ln("[demo] event logging enabled (backend: text)");
  }

  terminal.set_title(path.empty() ? "tui++ TextArea demo (in-memory document)" : "tui++ TextArea demo - " + path);
  terminal.set_type("text");

  std::shared_ptr<TextBuffer> buffer;
  if (path.empty()) {
    log_event_ln("[demo] generating the in-memory demo document");
    buffer = make_demo_buffer();
  } else {
    try {
      buffer = TextBuffer::open_file(path);
    } catch (std::exception const &ex) {
      std::fprintf(stderr, "%s\n", ex.what());
      return 1;
    }
  }

  auto frame = build_text_area_demo(buffer);
  (void)frame;
  terminal.run_event_loop();
}
