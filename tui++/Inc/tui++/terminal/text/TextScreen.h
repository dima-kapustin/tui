#pragma once

#include <vector>
#include <optional>

#include <tui++/Char.h>
#include <tui++/Color.h>
#include <tui++/Screen.h>
#include <tui++/Attributes.h>

#include <tui++/terminal/text/TextColor.h>

namespace tui {

class Terminal;
class TextGraphics;

class TextScreen: public Screen {
  using base = Screen;

  struct CharView {
    Char ch = ' ';
    Attributes attributes = Attributes::NONE;
    TextColor foreground_color = { };
    TextColor background_color = { };
  };

  static CharView EMPTY_CHAR_VIEW;

  std::vector</* rows */std::vector</* columns */CharView>> view;

  // The content of every cell as the terminal last saw it. A flush of a
  // region first compares the view against this shadow: rows whose damaged
  // span is unchanged emit nothing, and changed rows emit only their
  // differing runs, so repaints cost the actual difference and no-op
  // repaints cost nothing at all.
  std::vector</* rows */std::vector</* columns */CharView>> shadow;

  // True once a row has been emitted in full (its shadow cells are then
  // authoritative). Before that the whole row is compared and emitted, e.g.
  // after a resize that left the terminal content unknown.
  std::vector<bool> row_sent;

  // The SGR state the terminal is currently in (the last emitted cell, or
  // the empty state after a flush). Emitting switches from it to the target
  // state by deltas, so unchanged attributes emit no escapes even across
  // rows and repaint passes.
  CharView last_state;

  // Whether the current flush emitted any cell; the flush of the pass runs
  // only then (a repaint that changed nothing must not touch the terminal).
  bool emitted_any = false;

  std::shared_ptr<laf::LookAndFeel> look_and_feel;
  std::shared_ptr<TextMetrics> text_metrics;

private:
  TextScreen() noexcept;

  // Emits the damaged span of every row `region` touches, comparing the view
  // against the shadow first: identical rows are skipped, and a row with
  // changes emits only its differing runs (each positioned absolutely,
  // carrying the SGR state from the previous emission). Does not flush.
  void flush_rows(Rectangle const &region);

  // Ends the current flush: leaves the terminal with default attributes and
  // flushes it, but only when something was actually emitted.
  void end_flush();

  void escape_to(CharView const &cv);

  static bool same_cell(CharView const &a, CharView const &b);

  friend class Terminal;

private:
  void resize_view();

  TextColor to_terminal(Color const& c);

  void draw_char(Char ch, int x, int y, std::optional<Color> const &foreground_color, std::optional<Color> const &background_color, std::optional<Attributes> const &attributes) {
    auto &cv = this->view[y][x];
    cv.ch = ch;
    if (attributes) {
      cv.attributes = attributes.value();
    }
    if (foreground_color) {
      cv.foreground_color = to_terminal(foreground_color.value());
    }
    if (background_color) {
      cv.background_color = to_terminal(background_color.value());
    }
  }

  friend class TextGraphics;

public:
  void move_cursor_to(int line, int column);
  void move_cursor_by(int lines, int columns);

  virtual void run_event_loop() override;
  virtual std::unique_ptr<Graphics> get_graphics() override;
  virtual std::unique_ptr<Graphics> get_graphics(Rectangle const &clip) override;

  virtual std::shared_ptr<laf::LookAndFeel> get_look_and_feel() const override {
    return this->look_and_feel;
  }

  virtual std::shared_ptr<TextMetrics> get_text_metrics() const override {
    return this->text_metrics;
  }

  virtual void refresh();

  // Repaints only `rect` (screen coordinates): paints the component tree
  // with a graphics clipped to the region, then emits only what changed in
  // those rows (see flush_rows). The terminal flush is deferred to the end
  // of the repaint pass (Screen::repaint_damaged) and skipped when no cell
  // changed.
  virtual void repaint_region(Rectangle const &rect) override;

  virtual void repaint_pass_begin() override;
  virtual void repaint_pass_end() override;

  virtual void resized() override;

  void clear();
  void flush();
};

}

