// MenuBar demo for tui++.
//
// A standalone executable that builds ONE component tree -- a Frame whose
// root pane hosts a MenuBar (File/Edit popup menus) above a content panel --
// and renders it with either of the two terminal backends:
//
//     MenuBarDemo            text screen  (cell-based escape sequences)
//     MenuBarDemo text       same as the default
//     MenuBarDemo sixel      sixel screen (pixel-based graphics)
//
// The component tree itself never branches on the backend. The backend is
// selected first (terminal.set_type) and the tree is built afterwards, so
// every measurement it makes -- the screen size, the text metrics that turn
// "one cell" into the screen's units -- comes from whichever screen is
// active. One tree, two terminal types.
//
// Menu interaction is wired the way the font editor does it:
//   - a click on a top-level menu opens its popup and closes any other;
//   - a click on the content area dismisses the open popup;
//   - picking an item repaints the content panel in the next palette color
//     and echoes the choice in the terminal title.
//
// Quit with the File > Exit item or Ctrl+C.

#include <tui++/Event.h>
#include <tui++/BorderLayout.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Graphics.h>
#include <tui++/KeyStroke.h>
#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>
#include <tui++/Panel.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>
#include <tui++/border/EmptyBorder.h>
#include <tui++/border/LineBorder.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/util/log.h>
#include <tui++/util/typeid.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

using namespace tui;

namespace {

// Content palettes cycled by the menu items. The first entry reproduces the
// classic demo look (blue panel, heavy yellow border); the rest exist so a
// pick is visibly repainted on both backends.
struct Palette {
  Color fill;
  Color border;
};

constexpr Palette PALETTES[] = {
    { BLUE_COLOR, YELLOW_COLOR },
    { Color { 0x1B, 0x3B, 0x6F }, Color { 0xF4, 0xD0, 0x3F } },   // deep blue / gold
    { Color { 0x7B, 0x2C, 0xBF }, Color { 0xFF, 0xD6, 0x0A } },   // violet / amber
    { Color { 0x02, 0x3E, 0x8A }, Color { 0x90, 0xE0, 0xEF } },   // cobalt / ice
    { Color { 0x3D, 0x40, 0x5B }, Color { 0xE0, 0x7A, 0x5F } },   // slate / terracotta
    { Color { 0x38, 0x66, 0x41 }, Color { 0xF2, 0xE8, 0xCF } },   // pine / cream
};

// Mutable state shared by every menu action: the content panel the actions
// restyle, and the palette index they cycle through.
struct DemoState {
  std::shared_ptr<Panel> content;
  size_t palette_index = 0;
};

void apply_palette(const std::shared_ptr<DemoState> &state) {
  auto const &palette = PALETTES[state->palette_index % std::size(PALETTES)];
  state->content->set_background_color(palette.fill);
  state->content->set_border(std::make_shared<LineBorder>(Stroke::HEAVY, palette.border));
  state->content->repaint();
}

// Repaints the content in the next palette color and echoes the chosen menu
// item in the terminal title, so a pick is visible on both backends. `steps`
// lets neighbouring items land on different palettes.
void advance_palette(const std::shared_ptr<DemoState> &state, std::string_view item, size_t steps = 1) {
  state->palette_index = (state->palette_index + steps) % std::size(PALETTES);
  apply_palette(state);
  terminal.set_title("tui++ MenuBar demo - " + std::string { item });
}

// Creates an item of `menu` that closes the menu's popup before running
// `action`. The popup is opened directly (not through the MenuSelectionManager),
// so a pick has to dismiss it explicitly; the weak reference keeps the item
// from owning its menu. An optional `accelerator` (Swing's
// JMenuItem.setAccelerator) is shown right-aligned in the popup's accelerator
// column.
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

// A click on a top-level menu toggles its popup: picking a menu first closes
// every other menu's popup, clicking an already-open menu closes it again.
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
    // The item UI's do_click un-arms the menu after the release; the pointer
    // is still on the menu, so re-arm it to keep the hover highlight shown
    // while its popup stays open.
    self->set_armed(true);
    self->repaint();
    e.consume();
  });
}

// Diagnostic state shared by the hover tracker and the status panel.
struct HoverInfo {
  std::shared_ptr<Component> component;
  bool has_component = false;
  Point pointer { };
};

// Bottom status panel that reports the component currently under the mouse
// (its demangled type, name and id), its bounds, and the pointer position.
class HoverPanel: public Component {
  std::shared_ptr<HoverInfo> info;

  // Everything paint() draws, as one comparable string (the hovered
  // component's type, name and address, its showing state and bounds, and the
  // pointer position). update_if_changed() repaints only when this changes.
  std::string content_key() const {
    auto key = std::string { };
    if (this->info->has_component and this->info->component) {
      auto c = this->info->component;
      key = std::to_string(typeid(*c));
      key += '|';
      key += c->get_name();
      key += '|';
      key += std::to_string(reinterpret_cast<uintptr_t>(c.get()));
      if (c->is_showing()) {
        // get_location_on_screen throws for a component that is not showing,
        // so only include bounds while it is (the showing flag above tells
        // the two "hidden" states apart).
        auto bounds = Rectangle { c->get_location_on_screen(), c->get_size() };
        key += '|';
        key += std::to_string(bounds.x);
        key += ',';
        key += std::to_string(bounds.y);
        key += ',';
        key += std::to_string(bounds.width);
        key += ',';
        key += std::to_string(bounds.height);
      } else {
        key += "|-1"; // paint() shows "bounds=(component no longer showing)"
      }
    } else {
      key = "-";
    }
    key += '@';
    key += std::to_string(this->info->pointer.x);
    key += ',';
    key += std::to_string(this->info->pointer.y);
    return key;
  }

  std::string last_key;

public:
  explicit HoverPanel(const std::shared_ptr<HoverInfo> &info) :
      info(info) {
    set_opaque(true);
    set_background_color(Color { 24, 26, 34 });
    set_foreground_color(Color { 200, 200, 205 });
    // The panel is painted once when the frame is shown; seed the key with
    // that first content so the first mouse move does not repaint it again.
    this->last_key = content_key();
  }

  // Repaints only when the displayed content actually changed. A mouse move
  // that stays inside the same cell, or over an already-hovered component,
  // would otherwise schedule a repaint of identical rows on every event.
  void update_if_changed() {
    auto key = content_key();
    if (key != this->last_key) {
      this->last_key = std::move(key);
      repaint();
    }
  }

  void paint(Graphics &g) override {
    auto w = get_width();
    auto h = get_height();
    if (w <= 0 or h <= 0) {
      return;
    }

    auto metrics = screen.get_text_metrics();
    auto line = metrics->get_line_height();

    g.set_background_color(Color { 24, 26, 34 });
    g.fill_rect(0, 0, w, h);
    g.set_foreground_color(Color { 200, 200, 205 });

    auto y = 0;
    if (this->info->has_component and this->info->component) {
      auto c = this->info->component;
      auto name = c->get_name();
      char id[32];
      std::snprintf(id, sizeof id, "%p", static_cast<const void *>(c.get()));

      g.draw_string("hover: " + std::to_string(typeid(*c)) + "  name=\"" + (name.empty() ? "(unnamed)" : name) + "\"  id=" + id, 1, y);
      y += line;
      if (c->is_showing()) {
        // The popup menu can hide under a stationary pointer (picking an item
        // closes it without a MOUSE_EXITED), so the hovered component may be
        // gone by the time this panel repaints. get_location_on_screen throws
        // for a component that is not showing, so only ask for bounds while it
        // is; otherwise report that it is hidden instead of crashing.
        auto bounds = Rectangle { c->get_location_on_screen(), c->get_size() };
        g.draw_string("bounds=(x=" + std::to_string(bounds.x) + ", y=" + std::to_string(bounds.y) + ", w=" + std::to_string(bounds.width) + ", h=" + std::to_string(bounds.height) + ")", 1, y);
      } else {
        g.draw_string("bounds=(component no longer showing)", 1, y);
      }
      y += line;
    } else {
      g.draw_string("hover: (none)", 1, y);
      y += line;
    }
    g.draw_string("pointer=(x=" + std::to_string(this->info->pointer.x) + ", y=" + std::to_string(this->info->pointer.y) + ")", 1, y);
  }
};

// Screen listener that follows the synthesized mouse enter/exit events and
// the raw mouse movement/drag, keeping the hover panel current.
class HoverTracker: public EventListener<Event> {
  std::shared_ptr<HoverInfo> info;
  std::shared_ptr<HoverPanel> panel;

  // Updates the pointer position from a mouse event given in source-local
  // coordinates (Swing convention) and shows the window itself when no
  // interactive component is underneath.
  void track(MouseEvent &m) {
    if (auto c = std::dynamic_pointer_cast<Component>(m.source)) {
      this->info->pointer = convert_point_to_screen(m.x, m.y, c);
      if (not this->info->has_component) {
        // The pointer is over a dead area of a window (background, chrome,
        // a paint-only panel): reflect the window so the panel still shows
        // something meaningful instead of a stale component.
        this->info->component = c;
        this->info->has_component = true;
      }
    } else {
      this->info->pointer = { m.x, m.y };
    }
  }

  // Drops a hovered component that is no longer on screen. The popup menu can
  // hide under a stationary pointer -- picking an item closes its window
  // without a MOUSE_EXITED ever being synthesized -- so the tracked component
  // would otherwise stay "hovered" while hidden, and the hover panel's next
  // repaint would call get_location_on_screen() on it and crash.
  void drop_if_stale() {
    if (this->info->component and not this->info->component->is_showing()) {
      this->info->component = nullptr;
      this->info->has_component = false;
    }
  }

public:
  HoverTracker(const std::shared_ptr<HoverInfo> &info, const std::shared_ptr<HoverPanel> &panel) :
      info(info), panel(panel) {
  }

  void event_dispatched(Event &e) override {
    drop_if_stale();
    if (e.id == MouseOverEvent::MOUSE_ENTERED) {
      if (auto c = std::dynamic_pointer_cast<Component>(e.source)) {
        this->info->component = c;
        this->info->has_component = true;
        auto &m = static_cast<MouseOverEvent &>(e);
        this->info->pointer = convert_point_to_screen(m.x, m.y, c);
      }
      this->panel->update_if_changed();
    } else if (e.id == MouseOverEvent::MOUSE_EXITED) {
      if (auto c = std::dynamic_pointer_cast<Component>(e.source); c == this->info->component) {
        this->info->component = nullptr;
        this->info->has_component = false;
      }
      auto &m = static_cast<MouseOverEvent &>(e);
      if (auto c = std::dynamic_pointer_cast<Component>(e.source)) {
        this->info->pointer = convert_point_to_screen(m.x, m.y, c);
      } else {
        this->info->pointer = { m.x, m.y };
      }
      this->panel->update_if_changed();
    } else if (e.id == MouseMoveEvent::MOUSE_MOVED or e.id == MouseDragEvent::MOUSE_DRAGGED) {
      track(static_cast<MouseEvent &>(e));
      this->panel->update_if_changed();
    }
  }
};

// Builds the demo's component tree. Called after the backend has been
// selected, so the tree measures itself in the active screen's units: cells
// on the text screen, pixels on the sixel screen.
std::shared_ptr<Frame> build_menu_bar_demo() {
  auto state = std::make_shared<DemoState>();

  // Size the frame to the screen and inset the content so the frame's
  // background shows around the panel. The inset is expressed in cells via
  // the screen's text metrics: 2 cells on the text screen, 2 cell heights
  // (pixels) on the graphic screen.
  auto cell = screen.get_text_metrics()->get_line_height();

  auto frame = make_component<Frame>();
  frame->set_background_color(GREEN_COLOR);
  frame->set_size(screen.get_size());
  // Named for the event log (the frame has no title that would give it a
  // natural name).
  frame->set_name("main frame");

  auto content = make_component<Panel>();
  state->content = content;
  frame->add(content);
  content->set_name("content");
  frame->get_content_pane()->set_border(std::make_shared<EmptyBorder>(2 * cell, 2 * cell, 2 * cell, 2 * cell));
  apply_palette(state);

  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  add_item(file_menu, "New", 'N', KeyStroke { KeyEvent::VK_N, InputEvent::CTRL_DOWN }, [state] {
    advance_palette(state, "File: New");
  });
  add_item(file_menu, "Open", 'O', KeyStroke { KeyEvent::VK_O, InputEvent::CTRL_DOWN }, [state] {
    advance_palette(state, "File: Open");
  });
  add_item(file_menu, "Save", 'S', KeyStroke { KeyEvent::VK_S, InputEvent::CTRL_DOWN }, [state] {
    advance_palette(state, "File: Save");
  });
  file_menu->add_separator();
  add_item(file_menu, "Exit", 'x', std::nullopt, [] {
    terminal.shutdown();
  });

  auto edit_menu = make_component<Menu>("Edit");
  edit_menu->set_mnemonic('E');
  add_item(edit_menu, "Cut", 't', KeyStroke { KeyEvent::VK_X, InputEvent::CTRL_DOWN }, [state] {
    advance_palette(state, "Edit: Cut");
  });
  add_item(edit_menu, "Copy", 'C', KeyStroke { KeyEvent::VK_INSERT, InputEvent::CTRL_DOWN }, [state] {
    advance_palette(state, "Edit: Copy", 2);
  });
  add_item(edit_menu, "Paste", 'P', KeyStroke { KeyEvent::VK_V, InputEvent::CTRL_DOWN }, [state] {
    advance_palette(state, "Edit: Paste", 3);
  });
  edit_menu->add_separator();
  add_item(edit_menu, "Select All", 'A', KeyStroke { KeyEvent::VK_A, InputEvent::CTRL_DOWN }, [state] {
    advance_palette(state, "Edit: Select All");
  });
  add_item(edit_menu, "Delete", 'D', std::nullopt, [state] {
    advance_palette(state, "Edit: Delete");
  });

  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(file_menu);
  menu_bar->add(edit_menu);
  frame->set_menu_bar(menu_bar);
  menu_bar->set_name("menu bar");

  wire_menu_popup_toggle(file_menu, { edit_menu });
  wire_menu_popup_toggle(edit_menu, { file_menu });

  // A click on the content (that is, anywhere outside the open popup)
  // dismisses the popup again.
  content->add_listener([file_menu, edit_menu](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    for (auto &&menu : { file_menu, edit_menu }) {
      if (menu->is_popup_menu_visible()) {
        menu->set_popup_menu_visible(false);
      }
    }
  });

  // Diagnostic status bar: shows the component currently under the pointer,
  // its bounds and the pointer position (updated by the hover tracker below).
  auto hover_info = std::make_shared<HoverInfo>();
  auto hover_panel = make_component<HoverPanel>(hover_info);
  hover_panel->set_preferred_size(Dimension { 0, 3 * cell });
  frame->add(hover_panel, BorderLayout::SOUTH);
  hover_panel->set_name("hover panel");

  auto hover_tracker = std::make_shared<HoverTracker>(hover_info, hover_panel);
  screen.add_listener(EventType::MOUSE_MOVE | EventType::MOUSE_DRAG | EventType::MOUSE_OVER, hover_tracker);

  frame->set_visible(true);
  return frame;
}

int usage(const char *program) {
  std::fprintf(stderr,
               "usage: %s [--log-events] [text | sixel]\n"
               "\n"
               "MenuBar demo for tui++. Builds one component tree (a Frame with a\n"
               "File/Edit MenuBar over a content panel) and runs it on the selected\n"
               "terminal backend:\n"
               "  text          cell-based escape-sequence terminal (the default)\n"
               "  sixel         pixel-based sixel terminal\n"
               "  --log-events  write an ordered history of dispatched events and\n"
               "                repaints, with timings, to stderr (resize and\n"
               "                graphics-draw traces are logged as well)\n",
               program);
  return 1;
}

}

int main(int argc, char *argv[]) {
  // Select the rendering backend: "text" for the escape-sequence terminal
  // (the default) or "sixel" for the pixel-level sixel terminal. The screen
  // is created and themed by set_type; the component tree below is built
  // afterwards and is the same for both types. --log-events turns on the
  // library's event log (see tui++/Inc/tui++/util/log.h): every dispatched
  // event is logged by Component::dispatch_event, and Screen::dispatch_event /
  // Screen::repaint_damaged log per-event and per-repaint timings, so stderr
  // reads as an ordered history of events and their performance.
  auto trace = false;
  auto type = std::string_view { "text" };
  for (auto i = 1; i < argc; ++i) {
    auto arg = std::string_view { argv[i] };
    if (arg == "sixel" or arg == "text") {
      type = arg;
    } else if (arg == "--log-events" or arg == "-v") {
      trace = true;
    } else {
      return usage(argv[0]);
    }
  }

  if (trace) {
    util::event_log = &std::cerr;
    log_event_ln("[demo] event logging enabled (backend: " << type << ')');
  }

  terminal.set_title("tui++ MenuBar demo - " + std::string { type });
  terminal.set_type(type);

  auto frame = build_menu_bar_demo();
  terminal.run_event_loop();
}
