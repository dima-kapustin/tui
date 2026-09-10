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
  // A damaged band that is the terminal content shifted by a whole number of
  // rows is handled by flush_rows_by_terminal_scroll instead of the per-run
  // loop, so a wheel notch costs the rows that entered the band, not the
  // whole band.
  void flush_rows(Rectangle const &region);

  // Emits the differing runs of view row `y` over [first_column, last_column),
  // comparing against the shadow (which must hold the terminal's current
  // content for that row). Unchanged cells emit nothing; each changed run is
  // positioned absolutely and recorded in the shadow (see flush_rows).
  void emit_row(int y, int first_column, int last_column);

  // Emits the damaged span [first, last) of view row `y` as one contiguous
  // run, without the per-run shadow diff. Used when the whole span changed:
  // an edit that shifts a line's tail (typing at the start of a line) can
  // make many cells equal their shadow by chance (e.g. a repeated digit),
  // which the per-run diff would split into several cursor moves -- a visual
  // "snake". A single run redraws the row cleanly; the caller guarantees at
  // least one cell of the span changed.
  void emit_whole_row(int y, int first, int last);

  // Handles a damaged band of whole rows [first_row, last_row) by scrolling
  // the terminal's own buffer: when the band's view content is the shadow
  // shifted vertically by a whole number of rows (the typical scroll), a
  // scroll-region delete/insert moves the terminal's content, and only the
  // rows that entered the band (and the few rows that disagree with a pure
  // shift) are emitted. Returns true when the whole band was handled by the
  // scroll; false leaves the band to the per-run emission of flush_rows.
  // [first_column, last_column) is the damaged column span: the blank the
  // terminal paints on the revealed edge must match it cell for cell, or the
  // scroll is refused (see the uniformity check inside).
  bool flush_rows_by_terminal_scroll(int first_row, int last_row, int first_column, int last_column);

  static bool row_equals(std::vector<CharView> const &a, std::vector<CharView> const &b, int width);

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

  // Shifts the colors of every cell of `rect` towards `color` by `opacity`:
  // the cells keep their content -- the glyphs, the layout, the background --
  // and only darken (or lighten, with a light color). A cell color the
  // program never set is the terminal's own default (see set_default_colors),
  // which can only be shaded when the terminal told the screen what it is;
  // until then such a cell is left as it is. The primitive drop shadows are
  // painted with (see TextGraphics::blend_rect).
  void blend_rect(Rectangle const &rect, Color const &color, double opacity);

  // The colors the terminal shows for a cell the program never painted (its
  // OSC 10/11 answers, asked for when the event loop starts).
  std::optional<Color> default_foreground;
  std::optional<Color> default_background;

  friend class TextGraphics;

public:
  void move_cursor_to(int line, int column);
  void move_cursor_by(int lines, int columns);

  // Tells the screen what the terminal shows for a cell the program never
  // painted; the shadows blend such cells against these colors (see
  // blend_rect). run_event_loop fills them from the terminal's own OSC 10/11
  // answers; a program that drives the screen itself can set them here.
  void set_default_colors(std::optional<Color> const &foreground, std::optional<Color> const &background) {
    this->default_foreground = foreground;
    this->default_background = background;
  }

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

  // Drops the removed windows' cells from the view: their content was painted
  // into the same back buffer the remaining windows paint into, so the
  // repaint of those windows only overwrites the cells they draw. Resetting
  // the area to the empty state lets the following flush emit the erasure
  // (see Screen::on_window_removed).
  virtual void on_window_removed(Rectangle const &area) override;

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

