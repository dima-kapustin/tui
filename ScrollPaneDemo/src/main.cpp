// ScrollPane demo for tui++.
//
// A standalone executable that exercises the JScrollPane / JViewport /
// JScrollBar machinery of tui++ on the text backend:
//
//     ScrollPaneDemo             text screen (cell-based escape sequences)
//     ScrollPaneDemo --log-events  ... with the event/resize/graphics log on
//
// The window hosts ONE ScrollPane whose viewport shows a LogView: a large
// synthetic log (2000 rows x 132 columns) implementing Swing's Scrollable
// contract. Both scroll bars are therefore visible on a typical terminal and
// can be driven with the mouse (arrow cells, track clicks, thumb drags, the
// wheel) and the keyboard (cursor + page keys, F6/F7 cycle the bar policies).
// The status line under the pane mirrors the scroll bar models.
//
// Quit with Ctrl+C (or close the terminal).

#include <tui++/BorderLayout.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Graphics.h>
#include <tui++/Panel.h>
#include <tui++/Screen.h>
#include <tui++/ScrollPane.h>
#include <tui++/Scrollable.h>
#include <tui++/Viewport.h>
#include <tui++/TextMetrics.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/util/log.h>

#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace tui;

namespace {

constexpr int LOG_ROWS = 2000; // content rows of the log
constexpr int LOG_COLS = 132;  // content columns of the log

std::string log_line_text(int row) {
  // A synthetic service-log line, padded to LOG_COLS so the pane needs a
  // horizontal scroll bar as well.
  char buffer[LOG_COLS + 1];
  std::snprintf(buffer, sizeof buffer,
                "2026-09-07 13:%02d:%02d.%03d [node-%02d/worker-%02d] 0x%08x %s seq=%d payload=%-*s",
                (row * 7) % 60, (row * 13) % 60, (row * 37) % 1000, row % 12, row % 40,
                unsigned(row * 2654435761u), (row % 5 == 0) ? "ERROR" : (row % 3 == 0) ? "WARN " : "INFO ",
                row, LOG_COLS - 99, "x");
  std::string text(buffer);
  // Padded rows: keep the trailing spaces as real cells so the horizontal
  // extent of every row is the same LOG_COLS.
  text.resize(std::size_t(LOG_COLS), ' ');
  return text;
}

class LogView;

// The framework dispatches key events to the focused window, not to the focus
// owner; this forwarder (registered on the owning window) delivers them to the
// log view while the view is the focus owner.
class KeyToLogView final: public EventListener<KeyEvent> {
  std::weak_ptr<LogView> view;

public:
  explicit KeyToLogView(std::weak_ptr<LogView> const &view) :
      view(view) {
  }

  void key_pressed(KeyEvent &e) override;

  void key_typed(KeyEvent &) override {
  }
};

// The component shown in the pane's viewport: a synthetic log that is bigger
// than any plausible terminal, so the pane must scroll it both ways. Implements
// the Scrollable contract (unit = one row/column, block = a viewport page,
// neither dimension tracks the viewport), paints only the visible rows and
// columns, and drives the viewport like Swing's scrollRectToVisible when the
// cursor moves.
class LogView: public Component, public Scrollable {
  using base = Component;

public:
  LogView() {
    set_background_color(Color { 18, 20, 28 });
    set_foreground_color(Color { 150, 158, 172 });
    set_name("log view");
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  // Called by make_component once the view is owned by a shared pointer:
  // listeners are registered here (add_listener needs shared ownership).
  void init() override {
    base::init();
    add_listener([this](MousePressEvent &e) {
      if (e.id != MousePressEvent::MOUSE_PRESSED) {
        return;
      }
      // The coordinates are in this view's (file) space: the viewport places
      // the view at -view_position.
      if (e.y >= 0) {
        this->cursor_line = std::min(e.y, LOG_ROWS - 1);
        scroll_cursor_to_visible();
        repaint();
      }
      request_focus(false, FocusEvent::Cause::ACTIVATION);
      e.consume();
    });

    add_listener([this](MouseWheelEvent &e) {
      if (auto viewport = get_viewport()) {
        auto position = viewport->get_view_position();
        // One wheel notch scrolls one line (1:1), Swing's unit scrolling.
        viewport->set_view_position(position.x, position.y + e.wheel_rotation);
        e.consume();
      }
    });
  }

  // Key handling (delivered by KeyToLogView below while focus is on this view).
  void handle_key(KeyEvent &e) {
    auto ctrl = bool(e.modifiers & InputEvent::CTRL_DOWN);
    auto viewport = get_viewport();
    switch (e.get_key_code()) {
    case KeyEvent::VK_UP:
      move_cursor(-1, 0);
      break;
    case KeyEvent::VK_DOWN:
      move_cursor(1, 0);
      break;
    case KeyEvent::VK_LEFT:
      if (ctrl) {
        scroll_by(0, -LOG_COLS);
      } else {
        scroll_by(0, -8);
      }
      break;
    case KeyEvent::VK_RIGHT:
      if (ctrl) {
        scroll_by(0, LOG_COLS);
      } else {
        scroll_by(0, 8);
      }
      break;
    case KeyEvent::VK_PAGE_UP:
      move_cursor(-page_step(), 0);
      break;
    case KeyEvent::VK_PAGE_DOWN:
      move_cursor(page_step(), 0);
      break;
    case KeyEvent::VK_HOME:
      if (ctrl) {
        move_cursor(-LOG_ROWS, -LOG_COLS);
      } else if (viewport) {
        auto position = viewport->get_view_position();
        viewport->set_view_position(0, position.y);
      }
      break;
    case KeyEvent::VK_END:
      if (ctrl) {
        move_cursor(LOG_ROWS, LOG_COLS);
      } else if (viewport) {
        auto position = viewport->get_view_position();
        viewport->set_view_position(position.x, LOG_ROWS);
      }
      break;
    case KeyEvent::VK_F6:
      cycle_vertical_policy();
      break;
    case KeyEvent::VK_F7:
      cycle_horizontal_policy();
      break;
    default:
      return;
    }
    e.consume();
  }

  int get_cursor_line() const {
    return this->cursor_line;
  }

  std::shared_ptr<Viewport> get_viewport() const {
    for (auto parent = get_parent(); parent; parent = parent->get_parent()) {
      if (auto viewport = std::dynamic_pointer_cast<Viewport>(parent)) {
        return viewport;
      }
    }
    return {};
  }

  // Scrollable --------------------------------------------------------------

  Dimension get_preferred_scrollable_viewport_size() const override {
    return { 100, 30 };
  }

  int get_scrollable_unit_increment(Rectangle const &, Orientation) const override {
    return 1;
  }

  int get_scrollable_block_increment(Rectangle const &visible_rect, Orientation orientation) const override {
    return orientation == Orientation::VERTICAL ? std::max(1, visible_rect.height - 1) : std::max(1, visible_rect.width - 1);
  }

  bool get_scrollable_tracks_viewport_width() const override {
    return false;
  }

  bool get_scrollable_tracks_viewport_height() const override {
    return false;
  }

protected:
  void add_notify() override {
    if (auto window = get_containing_window()) {
      window->add_listener(std::make_shared<KeyToLogView>(std::static_pointer_cast<LogView>(shared_from_this())));
    }
    base::add_notify();
  }

  void paint(Graphics &g) override {
    auto width = get_width();
    auto height = get_height();
    if (width <= 0 or height <= 0) {
      return;
    }
    auto bg = get_background_color();
    if (bg) {
      g.set_background_color(bg);
      g.fill_rect(0, 0, width, height);
    }

    auto viewport = get_viewport();
    auto top = 0;
    auto left = 0;
    auto rows = height;
    auto columns = width;
    if (viewport) {
      auto position = viewport->get_view_position();
      top = std::max(0, position.y);
      left = std::max(0, position.x);
      rows = std::min(rows, viewport->get_height());
      columns = std::min(columns, viewport->get_width());
    }

    for (auto row = 0; row < rows; ++row) {
      auto line = top + row;
      if (line >= LOG_ROWS) {
        break;
      }
      auto text = log_line_text(line);
      auto visible = columns > 0 and left < int(text.size())
          ? text.substr(std::size_t(left), std::size_t(std::min(columns, int(text.size()) - left)))
          : std::string { };
      if (line == this->cursor_line) {
        g.set_background_color(Color { 52, 70, 108 });
        g.set_foreground_color(Color { 230, 235, 245 });
      } else if ((line % 10) == 0) {
        g.set_background_color(Color { 24, 27, 38 });
      } else {
        g.set_background_color(bg);
      }
      if (visible.empty()) {
        continue;
      }
      // The view child of the viewport sits at (-view_position) and spans the
      // whole content (see Viewport::place_view), so log line `line` lives at
      // view row `line`; only that row intersects the viewport's clip.
      g.draw_string(visible, 0, line);
      // The cursor row is highlighted across the whole visible width.
      if (line == this->cursor_line) {
        auto rest = std::size_t(std::max(0, columns - int(visible.size())));
        while (rest-- > 0) {
          g.draw_char(Char(' '), columns - int(rest) - 1, line);
        }
      }
    }
  }

private:
  friend class KeyToLogView;

  int page_step() const {
    auto viewport = get_viewport();
    return std::max(1, (viewport ? viewport->get_height() : get_height()) - 1);
  }

  void move_cursor(int row_delta, int column_delta) {
    auto viewport = get_viewport();
    if (not viewport) {
      return;
    }
    auto position = viewport->get_view_position();
    auto new_line = std::clamp(this->cursor_line + row_delta, 0, LOG_ROWS - 1);
    auto changed_row = new_line != this->cursor_line;
    this->cursor_line = new_line;
    if (column_delta != 0) {
      viewport->set_view_position(position.x + column_delta, position.y);
      position = viewport->get_view_position();
    }
    if (changed_row) {
      scroll_cursor_to_visible();
    } else {
      repaint();
    }
  }

  void scroll_by(int row_delta, int column_delta) {
    if (auto viewport = get_viewport()) {
      auto position = viewport->get_view_position();
      viewport->set_view_position(position.x + column_delta, position.y + row_delta);
    }
  }

  void scroll_cursor_to_visible() {
    if (auto viewport = get_viewport()) {
      viewport->scroll_rect_to_visible({ 0, this->cursor_line, 1, 1 });
    }
  }

  void cycle_vertical_policy() {
    for (auto parent = get_parent(); parent; parent = parent->get_parent()) {
      if (auto pane = std::dynamic_pointer_cast<ScrollPane>(parent)) {
        auto next = [](int policy) {
          return policy == ScrollPane::VERTICAL_SCROLLBAR_NEVER ? ScrollPane::VERTICAL_SCROLLBAR_AS_NEEDED : policy + 1;
        };
        pane->set_vertical_scroll_bar_policy(next(pane->get_vertical_scroll_bar_policy()));
        return;
      }
    }
  }

  void cycle_horizontal_policy() {
    for (auto parent = get_parent(); parent; parent = parent->get_parent()) {
      if (auto pane = std::dynamic_pointer_cast<ScrollPane>(parent)) {
        auto next = [](int policy) {
          return policy == ScrollPane::HORIZONTAL_SCROLLBAR_NEVER ? ScrollPane::HORIZONTAL_SCROLLBAR_AS_NEEDED : policy + 1;
        };
        pane->set_horizontal_scroll_bar_policy(next(pane->get_horizontal_scroll_bar_policy()));
        return;
      }
    }
  }

  int cursor_line = LOG_ROWS / 4;
};

// KeyToLogView implementation (defined after LogView is complete).
void KeyToLogView::key_pressed(KeyEvent &e) {
  if (not e.consumed) {
    if (auto view = this->view.lock(); view and KeyboardFocusManager::single->get_focus_owner() == view) {
      view->handle_key(e);
    }
  }
}

// The status line under the pane: mirrors the pane's scroll state. It
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

std::string policy_name(int policy) {
  switch (policy) {
  case ScrollPane::VERTICAL_SCROLLBAR_AS_NEEDED:
    return "as-needed";
  case ScrollPane::VERTICAL_SCROLLBAR_ALWAYS:
    return "always";
  default:
    return "never";
  }
}

struct DemoState {
  std::shared_ptr<ScrollPane> pane;
  std::shared_ptr<LogView> view;
  std::shared_ptr<StatusLine> status;
};

std::string status_text(DemoState const &state) {
  auto pane = state.pane;
  auto viewport = pane->get_viewport();
  auto position = viewport->get_view_position();
  auto v_model = pane->get_vertical_scroll_bar()->get_model();
  auto h_model = pane->get_horizontal_scroll_bar()->get_model();
  char buffer[320];
  std::snprintf(buffer, sizeof buffer,
                "pos(%d,%d)  vbar value=%d extent=%d max=%d %s  hbar value=%d extent=%d max=%d %s  cursor=%d  F6/F7: bar policies",
                position.x, position.y,
                v_model->get_value(), v_model->get_extent(), v_model->get_maximum(), policy_name(pane->get_vertical_scroll_bar_policy()).c_str(),
                h_model->get_value(), h_model->get_extent(), h_model->get_maximum(), policy_name(pane->get_horizontal_scroll_bar_policy()).c_str(),
                state.view->get_cursor_line());
  return buffer;
}

std::shared_ptr<Frame> build_scroll_pane_demo() {
  auto state = std::make_shared<DemoState>();

  auto frame = make_component<Frame>();
  frame->set_background_color(Color { 14, 16, 24 });
  frame->set_size(screen.get_size());
  frame->set_name("scroll pane demo frame");

  auto pane = make_component<ScrollPane>();
  state->pane = pane;
  pane->set_name("log scroll pane");
  pane->set_background_color(Color { 10, 12, 18 });
  frame->add(pane);

  auto view = make_component<LogView>();
  state->view = view;
  // The log's content size: the pane's layout resolves the view's content
  // from the preferred size when the view does not track a viewport axis.
  view->set_preferred_size(Dimension { LOG_COLS, LOG_ROWS });
  pane->set_viewport_view(view);

  // The status line below the pane; refreshed whenever a scroll bar model or
  // the keyboard changes something.
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

  auto refresh_status = [status] {
    status->refresh();
  };
  pane->get_vertical_scroll_bar()->get_model()->add_change_listener(refresh_status);
  pane->get_horizontal_scroll_bar()->get_model()->add_change_listener(refresh_status);
  refresh_status();

  // Keys that move the cursor without scrolling (arrows within the viewport)
  // do not touch the bar models; refresh the status after every key. Added
  // after set_visible so it runs after the log view's own key forwarder.
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
  frame->set_visible(true);
  frame->add_listener(std::make_shared<KeyRefresher>(refresh_status));
  return frame;
}

int usage(const char *program) {
  std::fprintf(stderr,
               "usage: %s [--log-events]\n"
               "\n"
               "ScrollPane demo for tui++ (text screen): a JScrollPane-style pane\n"
               "scrolling a 2000x132 synthetic log view. Keys: cursor, PgUp/PgDn,\n"
               "Ctrl+Home/End, F6/F7 cycle the bar policies. --log-events writes the\n"
               "event/resize/graphics history to stderr.\n",
               program);
  return 1;
}

} // namespace

int main(int argc, char *argv[]) {
  auto trace = false;
  for (auto i = 1; i < argc; ++i) {
    auto arg = std::string_view { argv[i] };
    if (arg == "--log-events" or arg == "-v") {
      trace = true;
    } else {
      return usage(argv[0]);
    }
  }

  if (trace) {
    util::event_log = &std::cerr;
    log_event_ln("[demo] event logging enabled (backend: text)");
  }

  terminal.set_title("tui++ ScrollPane demo");
  terminal.set_type("text");

  auto frame = build_scroll_pane_demo();
  (void)frame;
  terminal.run_event_loop();
}
