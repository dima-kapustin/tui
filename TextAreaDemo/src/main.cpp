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
// Cut/Copy/Paste, Delete, Select All, Show Invisibles). The Edit shortcuts
// live in the item's accelerator column, right-aligned as in Swing's menu
// layout. Selection:
// Shift+arrows / Shift+click; chords: Ctrl+X cut, Ctrl+Insert copy, Ctrl+V
// paste, Ctrl+A select all, Ctrl+Z / Ctrl+Y undo/redo (the console keeps
// Ctrl+C, so Copy uses Swing's secondary Ctrl+Insert binding). F3 search, F4
// regexp search, F5 Show Invisibles (whitespace dots/arrows), F6 caret form
// (block/underline), F7 caret blink (blink/steady/hidden). The status line
// mirrors the buffer and caret state.
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

#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace tui;

namespace {

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

struct DemoState {
  std::shared_ptr<ScrollPane> pane;
  std::shared_ptr<TextArea> area;
  std::shared_ptr<StatusLine> status;
};

// Menu helpers, following the MenuBarDemo wiring: an item's pick closes its
// popup and runs the action; a click on a top-level menu toggles its popup
// (opening one closes the others). An optional `accelerator` (Swing's
// JMenuItem.setAccelerator) gives the item a shortcut shown right-aligned in
// the popup's accelerator column.
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

void wire_menu_popup_toggle(const std::shared_ptr<Menu> &menu, const std::initializer_list<std::shared_ptr<Menu>> &others) {
  auto weak_self = std::weak_ptr<Menu> { menu };
  auto weak_others = std::vector<std::weak_ptr<Menu>> { };
  weak_others.reserve(others.size());
  for (auto &&other : others) {
    weak_others.emplace_back(other);
  }
  menu->add_listener([weak_self, weak_others](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_RELEASED) {
      return;
    }
    auto self = weak_self.lock();
    if (not self) {
      return;
    }
    auto open = not self->is_popup_menu_visible();
    if (open) {
      for (auto &&weak_other : weak_others) {
        if (auto other = weak_other.lock()) {
          other->set_popup_menu_visible(false);
        }
      }
    }
    self->set_popup_menu_visible(open);
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
                "row=%d  lines=%llu%s  caret=%llu (%llu:%llu)  bytes=%llu  ws=%s  regexp=%s",
                position.y,
                static_cast<unsigned long long>(buffer->known_line_count()),
                buffer->is_fully_scanned() ? " (scanned)" : "",
                static_cast<unsigned long long>(caret),
                static_cast<unsigned long long>(line),
                static_cast<unsigned long long>(column),
                static_cast<unsigned long long>(length),
                area->is_show_whitespace() ? "on" : "off",
                area->is_search_regexp() ? "on" : "off");
  return buffer_text;
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
  // move and after every key/mouse event that reaches the window.
  auto status = make_component<StatusLine>();
  state->status = status;
  status->set_preferred_size(Dimension { 0, 1 });
  status->set_background_color(Color { 8, 10, 15 });
  status->set_foreground_color(Color { 130, 140, 160 });
  status->set_name("status line");
  frame->add(status, BorderLayout::SOUTH);
  status->text = [state] {
    return status_text(*state);
  };
  auto refresh = [status] {
    status->refresh();
  };
  pane->get_vertical_scroll_bar()->get_model()->add_change_listener(refresh);
  pane->get_horizontal_scroll_bar()->get_model()->add_change_listener(refresh);
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
  (void)invisibles_item;

  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(file_menu);
  menu_bar->add(edit_menu);
  frame->set_menu_bar(menu_bar);
  menu_bar->set_name("menu bar");
  wire_menu_popup_toggle(file_menu, { edit_menu });
  wire_menu_popup_toggle(edit_menu, { file_menu });

  // A click on the content (outside the open popup) dismisses the menu.
  area->add_listener([file_menu, edit_menu](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    for (auto &&menu : { file_menu, edit_menu }) {
      if (menu->is_popup_menu_visible()) {
        menu->set_popup_menu_visible(false);
      }
    }
  });

  // The caret/scroll changes caused by mouse presses in the area.
  area->add_listener([refresh](MousePressEvent &e) {
    if (e.id == MousePressEvent::MOUSE_PRESSED) {
      refresh();
    }
  });

  frame->set_visible(true);
  frame->add_listener(std::make_shared<KeyRefresher>(refresh));
  // The caret's look is configurable, as a Swing text component's caret is:
  // F5 toggles the whitespace rendering (Edit > Show Invisibles), F6 cycles
  // the form (block/underline), F7 the blink mode
  // (blinking/steady/hidden); the message row reports the current choice.
  frame->add_listener([area, refresh, toggle_invisibles](KeyEvent &e) {
    if (e.id != KeyEvent::KEY_PRESSED) {
      return;
    }
    auto changed = false;
    switch (e.get_key_code()) {
    case KeyEvent::VK_F5:
      // The menu item's accelerator column shows the same shortcut.
      toggle_invisibles(area);
      refresh();
      e.consume();
      break;
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
               "text area implement Swing-style editing: Shift+arrows select,\n"
               "Ctrl+X cut, Ctrl+Insert copy, Ctrl+V paste, Ctrl+A select all,\n"
               "Ctrl+Z / Ctrl+Y undo/redo (the shortcuts are the menu items'\n"
               "accelerators, right-aligned in the Edit popup), F3 search (Enter\n"
               "jumps, F3 repeats), F4 regexp search, F5 Show Invisibles (the\n"
               "Edit menu's whitespace toggle), F6 caret form (block/underline),\n"
               "F7 caret blink (blinking/steady/hidden). --log-events writes the\n"
               "event/resize/graphics history to stderr.\n",
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
