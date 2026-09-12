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
// The Find sidebar floats inside the text view's upper right corner: it is a
// child of the viewport, pinned there by CornerLayout, so it overlaps the text
// instead of taking layout space from it. It stays hidden until a search asks
// for it (Ctrl+F, or F9 / the Find All button, which also fills the list), and
// Close hides it again. It holds the search pane -- the pattern field with the
// match options (case-insensitive, whole word and regexp) and the Find All and
// Close buttons -- over the Find All results list. Enter in the field runs the
// next match in the file view with those options; Find All lists every
// non-overlapping hit, each as a row "line:column  <text of the source line>";
// clicking a row or moving the caret in the list (arrows, PageUp/Down,
// Home/End) previews the hit in the file view, Enter previews and closes the
// sidebar, Esc closes it, F9 re-runs the search. The scans are byte-accurate
// and safe for UTF-8 (they never decode), windowed (bounded memory on huge
// files) and capped at FIND_ALL_LIMIT hits.
//
// Quit with Ctrl+C or the File menu.

#include <tui++/BorderLayout.h>
#include <tui++/BoxLayout.h>
#include <tui++/Button.h>
#include <tui++/CheckBox.h>
#include <tui++/Component.h>
#include <tui++/FlowLayout.h>
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
#include <tui++/TextField.h>
#include <tui++/Viewport.h>
#include <tui++/border/EmptyBorder.h>
#include <tui++/border/LineBorder.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/Layout.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/util/log.h>

#include <tui++/Panel.h>
#include <tui++/util/utf-8.h>

#include <cstdio>
#include <algorithm>
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

// The size of the Find sidebar in cells: the width the search pane's rows (the
// pattern field, the three options and the two buttons) need, and the height of
// the three control rows plus a dozen result rows. The sidebar is pinned inside
// the text view's upper right corner (see CornerLayout), so it overlaps the
// text instead of shrinking the text view.
constexpr int FIND_PANEL_WIDTH = 40;
constexpr int FIND_PANEL_HEIGHT = 16;

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

// A one-line static caption. The toolkit has no Label widget, and the search
// pane needs a prompt for its pattern field; this is the status line's painting
// without the refresh bookkeeping.
class Caption: public Component {
  std::string caption;

public:
  explicit Caption(std::string caption) :
      caption(std::move(caption)) {
    set_name("caption");
  }

protected:
  void paint(Graphics &g) override {
    g.set_foreground_color(get_foreground_color());
    g.draw_string(this->caption, 0, 0);
  }
};

// Pins a child of the container it is installed on to that container's upper
// right corner, at the child's preferred size (clamped to a smaller
// container). The Find sidebar is a child of the text view's viewport -- which
// has no layout of its own, it places its view directly -- so this only
// positions the sidebar; running with every layout pass is what re-pins it when
// the window resizes or a scroll bar appears.
class CornerLayout final: public AbstractLayout {
  std::shared_ptr<Component> child;

public:
  explicit CornerLayout(std::shared_ptr<Component> child) :
      child(std::move(child)) {
  }

  // Places the child at the target's upper right corner. The layout pass calls
  // it; the demo also calls it directly when the sidebar is shown, so the
  // sidebar is in place before the next paint instead of one pass later.
  void pin(std::shared_ptr<Component> const &target) {
    if (not this->child or not this->child->is_visible() or target->get_width() <= 0 or target->get_height() <= 0) {
      return;
    }
    auto preferred = this->child->get_preferred_size();
    auto width = std::clamp(preferred.width, 0, target->get_width());
    auto height = std::clamp(preferred.height, 0, target->get_height());
    this->child->set_bounds(target->get_width() - width, 0, width, height);
  }

  std::optional<Dimension> get_preferred_layout_size(std::shared_ptr<const Component> const &target) override {
    // The viewport's own size is its parent's decision (ScrollPaneLayout);
    // this layout only places the sidebar inside it.
    return Dimension { target->get_width(), target->get_height() };
  }

  std::optional<Dimension> get_minimum_layout_size(std::shared_ptr<const Component> const &target) override {
    return get_preferred_layout_size(target);
  }

  void layout(std::shared_ptr<Component> const &target) override {
    pin(target);
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

  // The Find sidebar at the text view's upper right: the search pane (the
  // pattern field, the match options and the buttons) and the read-only
  // results list under it, one hit per row (buffer row r corresponds to
  // hits[r]; see run_find_all).
  std::shared_ptr<Panel> find_panel;
  std::shared_ptr<CornerLayout> corner_layout;
  std::shared_ptr<TextField> search_field;
  std::shared_ptr<CheckBox> case_option;
  std::shared_ptr<CheckBox> word_option;
  std::shared_ptr<CheckBox> regex_option;
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

// The longest prefix of `text` of at most `max_bytes` bytes that ends on a
// UTF-8 character boundary, so a preview cut mid-sequence never lets a
// partial multi-byte character into the results list or the status line.
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

std::string status_text(DemoState const &state) {
  auto area = state.area;
  auto buffer = area->get_buffer();
  auto viewport = state.pane->get_viewport();
  auto position = viewport->get_view_position();
  auto v_model = state.pane->get_vertical_scroll_bar()->get_model();
  auto caret = area->get_caret();
  auto length = buffer->length();
  auto [line, column] = buffer->offset_to_line(caret);

  // The occurrence highlight: off, on, or on with a short preview of the text
  // it currently matches (a selection can be a whole phrase; the preview is
  // capped and UTF-8 safe, so the line stays one row and valid text).
  auto highlight = std::string { "off" };
  if (area->is_occurrence_highlight()) {
    auto text = area->get_occurrence_text();
    auto head = utf8_capped(text, 16);
    highlight = head.empty()
        ? std::string("on (no match)")
        : "on \"" + std::string(head) + (head.size() < text.size() ? "..." : "") + "\"";
  }

  char buffer_text[400];
  std::snprintf(buffer_text, sizeof buffer_text,
                "row=%d  lines=%llu%s  caret=%llu (%llu:%llu)  bytes=%llu  ws=%s  regexp=%s  hl=%s%s%s",
                position.y,
                static_cast<unsigned long long>(buffer->known_line_count()),
                buffer->is_fully_scanned() ? " (scanned)" : "",
                static_cast<unsigned long long>(caret),
                static_cast<unsigned long long>(line),
                static_cast<unsigned long long>(column),
                static_cast<unsigned long long>(length),
                area->is_show_whitespace() ? "on" : "off",
                area->is_search_regexp() ? "on" : "off",
                highlight.c_str(),
                area->is_column_select_mode() ? "  colmode=on" : "",
                area->is_block_selection() ? "  block" : "");
  // The hit count, once a search has filled the list (the results area's
  // buffer is empty before the first Find All).
  if (state.find_panel and state.find_panel->is_visible() and state.results_area->get_buffer()->length() > 0) {
    auto used = std::strlen(buffer_text);
    std::snprintf(buffer_text + used, sizeof buffer_text - used, "  hits=%llu%s",
                  static_cast<unsigned long long>(state.hits.size()),
                  state.find_capped ? " (capped)" : "");
  }
  return buffer_text;
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

// Hides the Find sidebar and returns the keyboard focus to the file view.
void close_find_panel(DemoState &state) {
  if (state.find_panel and state.find_panel->is_visible()) {
    state.find_panel->set_visible(false);
    state.find_panel->revalidate();
  }
  state.area->request_input_focus();
  state.status->refresh();
}

// Shows the Find sidebar -- it floats inside the text view and stays hidden
// until a search asks for it -- and pins it into the viewport's upper right
// corner right away; the viewport's CornerLayout keeps it pinned from then on.
void show_find_panel(DemoState &state) {
  state.find_panel->set_visible(true);
  if (auto viewport = state.pane->get_viewport(); viewport and state.corner_layout) {
    state.corner_layout->pin(viewport);
  }
}

// The pattern the Find All search uses: the sidebar's field, or the file area's
// F3 entry -- whose pattern survives the entry closing, so F9 keeps repeating
// the last typed search -- when the field is empty.
std::string find_pattern(DemoState const &state) {
  if (state.search_field and not state.search_field->get_text().empty()) {
    return state.search_field->get_text();
  }
  return state.area->get_search_pattern();
}

// Runs the find-all search and shows the sidebar. Every hit becomes one row of
// a fresh in-memory buffer shown by the read-only results area; the row text is
// the source line's first bytes (capped at a character boundary), so a log line
// tens of megabytes long cannot bloat the results buffer. The search honors the
// sidebar's options: case-insensitive and whole-word matching, and a regexp
// pattern (find_all_regex) instead of the plain substring scan.
void run_find_all(DemoState &state) {
  auto area = state.area;
  auto pattern = find_pattern(state);
  if (pattern.empty()) {
    // While the F3 entry is open the message row shows the entry, so only nag
    // when there is no entry to type in.
    if (not area->is_search_mode()) {
      area->show_message("no pattern yet: type one in the Find pane (Ctrl+F), or F3 in the view");
    }
    return;
  }
  if (area->is_search_mode()) {
    // Commit the entry: Find All finds all occurrences of the pattern typed so
    // far (the pattern stays, so the entry opens clean the next time).
    area->close_search_entry();
  }
  if (state.search_field and state.search_field->get_text().empty()) {
    // The search came from the F3 entry: show what it searched in the pane.
    state.search_field->set_text(pattern);
  }

  auto options = SearchOptions {
      .case_insensitive = state.case_option and state.case_option->is_selected(),
      .whole_word = state.word_option and state.word_option->is_selected() };
  auto regexp = state.regex_option and state.regex_option->is_selected();
  auto source = area->get_buffer();
  // Ask for one hit more than the cap so "capped" is exact (the list holds
  // FIND_ALL_LIMIT hits either way).
  auto matches = std::vector<std::pair<std::uint64_t, std::uint64_t>> { };
  if (regexp) {
    matches = source->find_all_regex(pattern, 0, FIND_ALL_LIMIT + 1, options);
  } else {
    auto offsets = source->find_all(pattern, 0, FIND_ALL_LIMIT + 1, options);
    matches.reserve(offsets.size());
    for (auto offset : offsets) {
      matches.emplace_back(offset, offset + pattern.size());
    }
  }
  state.find_capped = matches.size() > FIND_ALL_LIMIT;
  if (state.find_capped) {
    matches.resize(FIND_ALL_LIMIT);
  }

  state.hits.clear();
  state.hits.reserve(matches.size());
  auto rows = std::string();
  rows.reserve(matches.size() * 96);
  for (auto const &match : matches) {
    // The hits come sorted; the per-hit ensure_scanned_to is one forward
    // pass, and each hit's line then resolves without rescanning.
    source->ensure_scanned_to(match.first);
    auto [line, column] = source->offset_to_line(match.first);
    state.hits.push_back({ match.first, line, column });
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
  if (matches.empty()) {
    rows += std::string("(no ") + (regexp ? "regexp " : "") + "matches for \"" + pattern + "\")\n";
  } else if (state.find_capped) {
    rows += "(more hits than FIND_ALL_LIMIT: refine the pattern)\n";
  }

  auto results_buffer = TextBuffer::create_empty();
  results_buffer->replace(0, 0, rows);
  state.results_area->set_buffer(results_buffer);
  state.previewed = SIZE_MAX;
  show_find_panel(state);
  state.results_pane->get_viewport()->set_view_position(0, 0);
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

  // The occurrence-highlight toggle, shared by the F2 key and the Edit menu's
  // "Highlight Occurrences" item: every occurrence of the word under the
  // caret (or of the selection) that is visible on screen is highlighted.
  // The message reports the text the highlight is actually matching.
  auto toggle_occurrence_highlight = [](std::shared_ptr<TextArea> const &area) {
    area->set_occurrence_highlight(not area->is_occurrence_highlight());
    if (not area->is_occurrence_highlight()) {
      area->show_message("occurrence highlight off");
      return;
    }
    auto text = area->get_occurrence_text();
    area->show_message(text.empty()
        ? "occurrence highlight on: no word at the caret (move the caret onto a word, or select text)"
        : "occurrence highlight on: matching \"" + text + "\"");
  };

  auto frame = make_component<Frame>();
  frame->set_background_color(Color { 14, 16, 24 });
  frame->set_size(screen.get_size());
  frame->set_name("text area demo frame");

  auto pane = make_component<ScrollPane>();
  state->pane = pane;
  pane->set_name("text scroll pane");
  pane->set_background_color(Color { 10, 12, 18 });

  auto area = make_component<TextArea>();
  state->area = area;
  area->set_buffer(buffer);
  pane->set_viewport_view(area);

  // The status line below the text view. Refreshed when the scroll bar models
  // move and after every key/mouse event that reaches the window.
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

  // The Find sidebar: the search pane (the pattern field with the match
  // options, and the Find All and Close buttons) over the results list. It
  // floats inside the text component -- a child of the text view's viewport,
  // pinned to the viewport's upper right corner (see CornerLayout) -- rather
  // than taking layout space from it, and it starts hidden: Ctrl+F and F9 show
  // it, Close hides it again. The line border frames it as a rectangle of its
  // own, and its one-cell insets are what every row starts from, so all of
  // them (the pattern row, the options, the buttons and the results) line up
  // on the same column.
  auto find_panel = make_component<Panel>();
  state->find_panel = find_panel;
  find_panel->set_name("find panel");
  find_panel->set_layout(std::make_shared<BorderLayout>());
  find_panel->set_preferred_size(Dimension { FIND_PANEL_WIDTH, FIND_PANEL_HEIGHT });
  // The sidebar's face, shared with the widgets that blend into it.
  auto const find_face = Color { 14, 18, 28 };
  find_panel->set_background_color(find_face);
  find_panel->set_border(std::make_shared<LineBorder>(Stroke::LIGHT, Color { 70, 84, 110 }));
  find_panel->set_visible(false);

  // Row 1: the prompt and the pattern field (the field takes the rest of the
  // row).
  auto pattern_row = make_component<Panel>();
  pattern_row->set_name("find pattern row");
  pattern_row->set_layout(std::make_shared<BorderLayout>());
  // The prompt starts one cell in, the column the FlowLayout rows below start
  // their first widget at (their hgap), so the three control rows align.
  pattern_row->set_border(std::make_shared<EmptyBorder>(0, 1, 0, 1));
  auto prompt = make_component<Caption>("Find:");
  prompt->set_name("find prompt");
  prompt->set_foreground_color(Color { 170, 180, 200 });
  prompt->set_preferred_size(Dimension { 6, 1 });
  pattern_row->add(prompt, BorderLayout::WEST);
  auto search_field = make_component<TextField>();
  state->search_field = search_field;
  search_field->set_name("find pattern");
  pattern_row->add(search_field, BorderLayout::CENTER);

  // Row 2: the match options (SearchOptions plus the regexp mode).
  auto options_row = make_component<Panel>();
  options_row->set_name("find options");
  options_row->set_layout(std::make_shared<FlowLayout>(FlowLayout::LEFT, 1, 0));
  auto add_option = [&options_row, find_face](std::string const &text) {
    auto box = make_component<CheckBox>(text);
    // The theme gives a check box the system control face (light); on the
    // sidebar's dark panel that face would wash out the light label, so the
    // box blends with the panel, as the buttons below do.
    box->set_background_color(find_face);
    box->set_foreground_color(Color { 190, 196, 205 });
    options_row->add(box);
    return box;
  };
  state->case_option = add_option("Case");
  state->word_option = add_option("Whole word");
  state->regex_option = add_option("Regex");

  // Row 3: the actions.
  auto buttons_row = make_component<Panel>();
  buttons_row->set_name("find buttons");
  buttons_row->set_layout(std::make_shared<FlowLayout>(FlowLayout::LEFT, 1, 0));
  auto find_all_button = make_component<Button>("Find All");
  auto close_button = make_component<Button>("Close");
  for (auto button : { find_all_button, close_button }) {
    button->set_background_color(Color { 44, 54, 74 });
    button->set_foreground_color(Color { 214, 222, 236 });
    buttons_row->add(button);
  }

  // The three control rows, stacked by a vertical box: its preferred height
  // is their sum, so each row keeps its own height (a BorderLayout sizes the
  // panel to the tallest row and lets the others overlap) and every row is
  // stretched to the panel's width, which is what keeps them left-aligned.
  auto controls = make_component<Panel>();
  controls->set_name("find controls");
  controls->set_layout(std::make_shared<BoxLayout>(controls.get(), BoxLayout::Y));
  controls->add(pattern_row);
  controls->add(options_row);
  controls->add(buttons_row);

  // The results list: a read-only TextArea, one hit per row (see run_find_all,
  // whose rows carry the line, the column and a preview of the source line).
  auto results_pane = make_component<ScrollPane>();
  state->results_pane = results_pane;
  results_pane->set_name("find all results");
  results_pane->set_background_color(Color { 10, 12, 18 });
  // The results text starts on the same column as the control rows above it
  // (the ScrollPane lays its viewport out inside these insets).
  results_pane->set_border(std::make_shared<EmptyBorder>(0, 1, 0, 1));
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

  find_panel->add(controls, BorderLayout::NORTH);
  find_panel->add(results_pane, BorderLayout::CENTER);

  // The sidebar is opaque to the mouse: a press on its background (or on the
  // controls' gaps) must not reach the text behind it and move the file view's
  // caret. The viewport's hit-testing skips components that accept no mouse
  // events, so this listener is what claims the presses.
  find_panel->add_listener([](MousePressEvent &e) {
    e.consume();
  });

  // The text view fills the content; the sidebar floats inside its viewport,
  // above the text, and the status line sits under both.
  frame->add(pane, BorderLayout::CENTER);
  frame->add(status, BorderLayout::SOUTH);
  if (auto viewport = pane->get_viewport()) {
    auto corner = std::make_shared<CornerLayout>(find_panel);
    state->corner_layout = corner;
    viewport->set_layout(corner);
    // Index 0 is the top of the stack: the sidebar paints over the text view.
    viewport->add(find_panel, 0);
  }
  refresh();

  // The search pane's wiring. The two option boxes map to the area's
  // SearchOptions, so the area's own F3 search (find_next) honors them as
  // well; the regexp box is the mode the field's Enter and Find All pass
  // along.
  auto apply_options = [state] {
    state->area->set_search_options(SearchOptions {
        .case_insensitive = state->case_option and state->case_option->is_selected(),
        .whole_word = state->word_option and state->word_option->is_selected() });
  };
  for (auto option : { state->case_option, state->word_option }) {
    option->add_listener([apply_options](ActionEvent &) {
      apply_options();
    });
  }

  // Enter in the pattern field runs the next match in the file view (the F3
  // search's forward step) with the options; the field keeps the keyboard
  // focus, so the pattern can be refined without leaving it.
  search_field->add_listener([state, apply_options, refresh](ActionEvent &) {
    apply_options();
    auto pattern = state->search_field->get_text();
    if (pattern.empty()) {
      state->area->show_message("type a pattern to find");
      return;
    }
    auto regexp = state->regex_option and state->regex_option->is_selected();
    state->area->find_next(pattern, regexp, true);
    refresh();
  });

  find_all_button->add_listener([state](ActionEvent &) {
    run_find_all(*state);
  });
  close_button->add_listener([state](ActionEvent &) {
    close_find_panel(*state);
  });

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
  // The Find sidebar's toggle: Ctrl+F shows the pane and puts the keyboard
  // focus in the pattern field, as an editor's find chord does.
  auto search_pane_item = add_item(edit_menu, "Search Pane", 'S', KeyStroke { KeyEvent::VK_F, InputEvent::CTRL_DOWN }, [state, refresh] {
    show_find_panel(*state);
    state->search_field->request_input_focus();
    refresh();
  });
  // Find All opens the sidebar for the sidebar's pattern -- or the area's own
  // search pattern (the F3 entry's) when the field is empty. The F9 shortcut
  // is handled by the window key handler below, the way F5/F8 accelerators
  // are.
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
  // The view toggles (Swing leaves these to the application, so the
  // shortcuts are the items' accelerators rather than key handlers inside
  // TextArea).
  auto occurrences_item = add_item(edit_menu, "Highlight Occurrences", 'H', KeyStroke { KeyEvent::VK_F2, InputEvent::NO_MODIFIERS }, edit_action([toggle_occurrence_highlight](auto const &area) {
    toggle_occurrence_highlight(area);
  }));
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
  (void)occurrences_item;
  (void)search_pane_item;
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
  // items already carry as accelerators (F2 for Highlight Occurrences, F5 for
  // Show Invisibles, F8 for Column Select Mode, F9 for Find All, Ctrl+F for
  // the Search Pane) are left to them.
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
  // Find All keyboard handling: the sidebar's own navigation. F9 runs (or
  // re-runs) the search through its menu item's accelerator; a click or the
  // caret keys in the results area move its caret first (this listener runs
  // after the area's own key forwarder), so the row the caret landed on is
  // previewed in the file view; Enter previews and closes the sidebar; Esc
  // closes it from either text area. The sidebar's pattern field owns its
  // keys instead (Enter searches, the arrows move within the pattern).
  frame->add_listener([state](KeyEvent &e) {
    if (e.id != KeyEvent::KEY_PRESSED) {
      return;
    }
    auto results = state->results_area;
    auto results_open = state->find_panel and state->find_panel->is_visible();
    if (not results_open) {
      return;
    }
    auto row = results_caret_row(*state);
    if (not results->is_focus_owner()) {
      // The file area (or nothing) owns the focus: Esc dismisses the sidebar.
      if (e.get_key_code() == KeyEvent::VK_ESCAPE) {
        close_find_panel(*state);
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
      close_find_panel(*state);
      break;
    case KeyEvent::VK_ESCAPE:
      close_find_panel(*state);
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
               "F2 occurrence highlight: every visible occurrence of the word\n"
               "at the caret -- or of the selection, when there is one -- is\n"
               "highlighted. F3 search (Enter jumps, F3 repeats), F4 regexp\n"
               "search, F5 Show Invisibles (the Edit menu's whitespace toggle),\n"
               "F6 caret form (block/underline), F7 caret blink\n"
               "(blinking/steady/hidden), F8 column select mode. The Find sidebar\n"
               "floats inside the text view's upper right corner (Ctrl+F, or\n"
               "Edit > Search Pane, shows it and focuses the field): the pattern\n"
               "field with the Case (case-insensitive), Whole word and Regex\n"
               "options over the results list. Enter in the field runs the next\n"
               "match with the options; Find All (the button, F9, or Edit > Find\n"
               "All) lists every hit of the pattern as \"line:column  <source\n"
               "line>\": click a row or use the caret keys to preview the hit in\n"
               "the file view, Enter previews and closes the sidebar, Esc closes\n"
               "it, Close hides it (the scans are UTF-8 safe, windowed and capped\n"
               "at FIND_ALL_LIMIT hits). Double-clicking a word in the file view\n"
               "selects it (a triple-click takes the line). --log-events writes\n"
               "the event/resize/graphics history to stderr.\n",
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
